#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#include <linux/delay.h>
#include <linux/gpio/consumer.h> /* For GPIO Descriptor interface */
#include <linux/mutex.h>
#include <linux/of.h>              /* For DT*/
#include <linux/platform_device.h> /* For platform devices */

#include <linux/kfifo.h>
#include <linux/ktime.h>

#define MAX_SIZE 128

#define MODULE_NAME "onewire_dev"

#define PIN_ONEWIRE "onewire"

#define RESULT_FIFO_SIZE 128
#define BUFFER_SIZE 512

// Structure to hold device-specific data
struct onewire_dev
{
    char *kernel_buffer;
    int buffer_size;
    struct cdev cdev;
};

struct gpio_desc *onewire_pin;
struct gpio_desc *onewire_pin_in;
static struct onewire_dev *s_dev = NULL;
static int major_number = 0;
static struct class *cls;

// Data structures
struct read_data_t
{
    char data[8];
    size_t size;
};

bool resend_false_crc = false;

/**
 *  declare a static fifo
 *  According to source DOC the spinlock is not needed, since there is only on reader or writer
 * */
static DECLARE_KFIFO (result_fifo, struct read_data_t *, RESULT_FIFO_SIZE);

// define a mutex
static DEFINE_MUTEX (open_mutex);
static int device_opened;

const char bit_mask[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

// Lookup table for the 1-Wire CRC8
static const uint8_t onewire_crc8_table[256] = {
    0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83, 0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41,
    0x9D, 0xC3, 0x21, 0x7F, 0xFC, 0xA2, 0x40, 0x1E, 0x5F, 0x01, 0xE3, 0xBD, 0x3E, 0x60, 0x82, 0xDC,
    0x23, 0x7D, 0x9F, 0xC1, 0x42, 0x1C, 0xFE, 0xA0, 0xE1, 0xBF, 0x5D, 0x03, 0x80, 0xDE, 0x3C, 0x62,
    0xBE, 0xE0, 0x02, 0x5C, 0xDF, 0x81, 0x63, 0x3D, 0x7C, 0x22, 0xC0, 0x9E, 0x1D, 0x43, 0xA1, 0xFF,
    0x46, 0x18, 0xFA, 0xA4, 0x27, 0x79, 0x9B, 0xC5, 0x84, 0xDA, 0x38, 0x66, 0xE5, 0xBB, 0x59, 0x07,
    0xDB, 0x85, 0x67, 0x39, 0xBA, 0xE4, 0x06, 0x58, 0x19, 0x47, 0xA5, 0xFB, 0x78, 0x26, 0xC4, 0x9A,
    0x65, 0x3B, 0xD9, 0x87, 0x04, 0x5A, 0xB8, 0xE6, 0xA7, 0xF9, 0x1B, 0x45, 0xC6, 0x98, 0x7A, 0x24,
    0xF8, 0xA6, 0x44, 0x1A, 0x99, 0xC7, 0x25, 0x7B, 0x3A, 0x64, 0x86, 0xD8, 0x5B, 0x05, 0xE7, 0xB9,
    0x8C, 0xD2, 0x30, 0x6E, 0xED, 0xB3, 0x51, 0x0F, 0x4E, 0x10, 0xF2, 0xAC, 0x2F, 0x71, 0x93, 0xCD,
    0x11, 0x4F, 0xAD, 0xF3, 0x70, 0x2E, 0xCC, 0x92, 0xD3, 0x8D, 0x6F, 0x31, 0xB2, 0xEC, 0x0E, 0x50,
    0xAF, 0xF1, 0x13, 0x4D, 0xCE, 0x90, 0x72, 0x2C, 0x6D, 0x33, 0xD1, 0x8F, 0x0C, 0x52, 0xB0, 0xEE,
    0x32, 0x6C, 0x8E, 0xD0, 0x53, 0x0D, 0xEF, 0xB1, 0xF0, 0xAE, 0x4C, 0x12, 0x91, 0xCF, 0x2D, 0x73,
    0xCA, 0x94, 0x76, 0x28, 0xAB, 0xF5, 0x17, 0x49, 0x08, 0x56, 0xB4, 0xEA, 0x69, 0x37, 0xD5, 0x8B,
    0x57, 0x09, 0xEB, 0xB5, 0x36, 0x68, 0x8A, 0xD4, 0x95, 0xCB, 0x29, 0x77, 0xF4, 0xAA, 0x48, 0x16,
    0xE9, 0xB7, 0x55, 0x0B, 0x88, 0xD6, 0x34, 0x6A, 0x2B, 0x75, 0x97, 0xC9, 0x4A, 0x14, 0xF6, 0xA8,
    0x74, 0x2A, 0xC8, 0x96, 0x15, 0x4B, 0xA9, 0xF7, 0xB6, 0xE8, 0x0A, 0x54, 0xD7, 0x89, 0x6B, 0x35
};

/**
 * Adds a response to the FIFO
 */
static write_response_char(char c) {
    struct read_data_t *result = kmalloc (sizeof (struct read_data_t), GFP_KERNEL);
    for (int i = 0; i < 8; i++)
    {
        result->data[i] = 0;
    }
    result->data[0] = c;
    result->size = 8;

    kfifo_put (&result_fifo, result); 
}

/**
 * Compares 2 strings
 * returns 0 on failure, 1 on success
 */
static int
string_cmp (const char *s1, const char *s2, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        if (s1[i] != s2[i])
        {
            return 0;
        }
    }
    return 1;
}

/**
 * compute the 1-Wire CRC via lookup table
 */
static uint8_t
compute_crc (const uint8_t *data, size_t len)
{
    uint8_t crc = 0x00;
    while (len--)
    {
        crc = onewire_crc8_table[crc ^ *data];
        data++;
    }
    return crc;
}

/**
 * Write data to the 1-Wire lane
 */
static int
write_cmd (struct gpio_desc *request, char *data, size_t length)
{
    printk ("Write CMD \n");

    gpiod_direction_input (request);
    // iterate over each byte
    for (int i = 0; i < length; i++)
    {
        // iterate over each bit
        for (int j = 0; j < 8; j++)
        {
            unsigned long flags;

            if (data[i] & bit_mask[j])
            {
                local_irq_save (flags);
                // printk ("Write 1 \n");
                gpiod_direction_output (request, 0);
                udelay (7);

                gpiod_direction_input (request);
                local_irq_restore (flags);
                udelay (60);
            }
            else
            {
                local_irq_save (flags);
                // printk ("Write 0 \n");
                gpiod_direction_output (request, 0);
                udelay (60);
                gpiod_direction_input (request);
                local_irq_restore (flags);
                udelay (15);
            }
        }
        udelay (30);
    }
    gpiod_direction_input (request);

    return 0;
}

/**
 * Read data from the 1-Wire lane,
 * Reads exactly length*8 bits
 * "data" format is little endian
 */
static uint8_t
read_cmd (struct gpio_desc *request, char *data, size_t length)
{
    printk("read_cmd \n");
    unsigned long flags;
    local_irq_save (flags);

    for (int i = 0; i < length; i++)
    {
        char read_bits = 0;
        for (int j = 0; j < 8; j++)
        {

            gpiod_direction_output (request, 0);
            udelay (9);

            gpiod_direction_input (request);

            udelay (15);
            int rd = gpiod_get_value (request);

            if (rd == 0)
            {
                printk ("Read 0 \n");
                read_bits = read_bits >> 1;
            }
            else
            {
                printk ("Read 1 \n");
                read_bits = (read_bits >> 1) | 0x80; // put a '1' at bit 7
            }

            udelay (60);
        }
        data[i] = read_bits;
        printk ("------ readd data %x  -------------\n", read_bits);
    }
    local_irq_restore (flags);
    gpiod_direction_input (request);

    for (int i = 0; i < length; i++)
    {
        printk ("%x ", data[i]);
    }
    uint8_t crc = compute_crc (data, length - 1);

    uint8_t res = crc == data[length - 1];
    return res;
}

/**
 * 1-Wire reset
 * Pull down for 500 US
 */
static void
reset (struct gpio_desc *request)
{
    printk ("run reset \n");

    gpiod_direction_output (request, 1);
    gpiod_set_value (request, 0);

    udelay (500);

    gpiod_set_value (request, 1);

    gpiod_direction_input (request);

    // int ret = wait_until_rising_edge (request);
    // printk ("ret wait %i \n", ret);

    udelay (500);
}

/**
 * Handles the device open operation
 * locks the driver from accesses to other processes
 */
static int
onewire_open (struct inode *inode, struct file *filp)
{
    printk (KERN_INFO "%s: Device opened\n", MODULE_NAME);

    // check if the device is already opened
    if (mutex_lock_interruptible (&open_mutex))
        return -ERESTARTSYS;

    if (device_opened)
    {
        mutex_unlock (&open_mutex);
        return -EBUSY;
    }

    // grab the device
    device_opened = true;
    mutex_unlock (&open_mutex);

    return 0;
}

/**
 * Handles the device release operation
 * frees the driver lock
 */
static int
onewire_release (struct inode *inode, struct file *filp)
{
    printk (KERN_INFO "%s: Device released\n", MODULE_NAME);

    // release the device
    mutex_lock (&open_mutex);
    device_opened = false;
    mutex_unlock (&open_mutex);

    return 0;
}

/**
 * Reads an element from the KFIFO and returns its content to the user space
 */
static ssize_t
onewire_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    printk ("READ \n");

    printk ("f_pos %p count %zu \n", f_pos, count);

    // read the fifo data
    struct read_data_t *result;
    int processed_elements = kfifo_get (&result_fifo, &result);
    printk ("ptr_adr %p processed_elements %i \n", result, processed_elements);
    if (processed_elements == 0)
    { // if fifo was empty stop reading
        return 0;
    }

    // copy the fifo to the buffer
    // result->size is always < 8
    // so always enough data in the kernl_buffer
    for (size_t i = 0; i < result->size; i++)
    {
        printk ("c %x ", result->data[i]);
        s_dev->kernel_buffer[i] = result->data[i];
    }

    // boundary checks
    size_t min_count = min (count, result->size);
    if (count < result->size)
    {
        pr_warn ("%s: userspace buffer is too small (%zu < %zu)\n", MODULE_NAME, count, min_count);
    }

    printk ("result size %u min_count %u \n", result->size, min_count);
    if (copy_to_user (buf, s_dev->kernel_buffer, min_count))
    {
        return -EFAULT; // Failed to copy to user space
    }

    kfree (result);

    *f_pos += min_count;
    return min_count;
}

/**
 * Handles the write operation from user space
 * It interprets the characters and runs the according operation
 * not all operations are 1-Wire related
 */
static ssize_t
onewire_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t bytes_written = 0;

    pr_info ("copy count %u f_pos %llu \n", count, *f_pos);

    // verify there is not more written than buffer space is available
    if (count > s_dev->buffer_size - *f_pos)
    {
        count = s_dev->buffer_size - *f_pos;
    }

    if (copy_from_user (s_dev->kernel_buffer + *f_pos, buf, count))
    {
        return -EFAULT; // Failed to copy from user space
    }

    pr_info ("copy_from user space scceeded\n");
    *f_pos += count;
    bytes_written = count;

    if (kfifo_is_full (&result_fifo))
    {
        pr_err ("Error kenel fifo is full. Can not write data");
        return -EFAULT;
    }

    if (count > 0)
    {
        if (s_dev->kernel_buffer[0] == 'r')
        {
            reset (onewire_pin);
            write_response_char('r');
        }
        else if (s_dev->kernel_buffer[0] == 'h')
        {
            gpiod_direction_output (onewire_pin, 1);
            gpiod_set_value (onewire_pin, 1);
            write_response_char('h');
        }
        else if (s_dev->kernel_buffer[0] == 'l')
        {
            gpiod_direction_output (onewire_pin, 0);
            gpiod_set_value (onewire_pin, 0);
            write_response_char('l');
        }
        else if (s_dev->kernel_buffer[0] == 'i')
        {
            gpiod_direction_input (onewire_pin);
            write_response_char('i');
        }
        else if (string_cmp (s_dev->kernel_buffer, "ECRC", 4))
        {
            printk ("Enable CRC check \n");
            resend_false_crc = true;
            write_response_char('1');
        }
        else if (string_cmp (s_dev->kernel_buffer, "DCRC", 4))
        {
            printk ("Disable CRC check \n");
            resend_false_crc = false;
            write_response_char('1');
        }
        else if (string_cmp (s_dev->kernel_buffer, "FLUSH", 5)) // Flush the FIFO
        {
            printk ("FLUSH \n");
            printk ("KFIFO length %u \n", kfifo_len (&result_fifo));
            unsigned int kfifo_len = kfifo_len (&result_fifo);

            // free all memory of the pointer elements
            struct read_data_t *result = NULL;
            for (unsigned int off = 0; off < kfifo_len; ++off)
            {
                if (kfifo_get (&result_fifo, &result) > 0)
                {
                    kfree (result);
                }
                else
                {
                    pr_err ("Error in 'FLUSH' can not free at iteration index %u ", off);
                }
            }

            printk ("KFIFO length %u \n", kfifo_len (&result_fifo));
        }
        else if (string_cmp (s_dev->kernel_buffer, "SIZE", 4)) // Get FIFO size
        {
            unsigned int kfifo_len = kfifo_len (&result_fifo);
            printk ("KFIFO length %u \n", kfifo_len (&result_fifo));

            // read all fifo items and free its allocated memory
            struct read_data_t *result = kmalloc (sizeof (struct read_data_t), GFP_KERNEL);
            for (int i = 0; i < 8; i++)
            {
                result->data[i] = 0;
            }
            result->size = 8;
            result->data[0] = kfifo_len & 0xFF;
            result->data[1] = kfifo_len & 0xFF00;
            result->data[2] = kfifo_len & 0xFF0000;
            result->data[3] = kfifo_len & 0xFF000000;

            kfifo_put (&result_fifo, result);
        }
        else if (string_cmp (s_dev->kernel_buffer, "RA", 2)) // Read Address
        {
            printk ("Read Address \n");
            reset (onewire_pin);

            // char data[2] = { 0xCC, 0xBE };
            char data[1] = { 0x33 };
            write_cmd (onewire_pin, data, 1);

            udelay (500);
            char data_read[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

            if (resend_false_crc)
            {
                int crc_correct = 0;
                for (int i = 0; i < 20; i++)
                {
                    crc_correct = read_cmd (onewire_pin, data_read, 8);
                    if (crc_correct)
                        break;
                    else
                        msleep (1000);
                }
            }
            else
            {
                read_cmd (onewire_pin, data_read, 8);
            }

            struct read_data_t *result = kmalloc (sizeof (struct read_data_t), GFP_KERNEL);
            for (int i = 0; i < 8; i++)
            {
                result->data[i] = data_read[i];
            }
            result->size = 8;

            printk ("prt adr %p &adr %p \n", result, &result);
            kfifo_put (&result_fifo, result);
        }
        /**
         * Write scratchpad gets 3 addtional bytes
         */
        else if (string_cmp (s_dev->kernel_buffer, "WS", 2)) // Write Scrathpad
        {
            reset (onewire_pin);

            char data[7];
            data[0] = 0xCC;
            data[1] = 0x4E;
            for (int i = 0; i < min (3, count - 2); i++)
            {
                data[i + 2] = s_dev->kernel_buffer[i + 2];
            }
            write_cmd (onewire_pin, data, sizeof (data));

            udelay (600);
        }
        else if (string_cmp (s_dev->kernel_buffer, "RS", 2)) // Read Scrathpad
        {
            printk ("Read scratchpad \n");
            reset (onewire_pin);

            char data[2];
            data[0] = 0xCC;
            data[1] = 0xBE;
            write_cmd (onewire_pin, data, 2);

            udelay (600);

            char data_read[9] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0, 0x0 };
            int crc_correct = 0;
            if (resend_false_crc)
            {
                for (int i = 0; i < 20; i++)
                {
                    crc_correct = read_cmd (onewire_pin, data_read, 8 + 1);
                    printk ("computed crc %d \n", crc_correct);
                    if (crc_correct)
                        break;
                    else
                        msleep (1000);
                }
            }
            else
            {
                crc_correct = read_cmd (onewire_pin, data_read, 8 + 1);
                printk ("computed crc %d \n", crc_correct);
            }

            struct read_data_t *result = kmalloc (sizeof (struct read_data_t), GFP_KERNEL);
            for (int i = 0; i < 8; i++)
            {
                result->data[i] = data_read[i];
            }
            result->size = 8;

            kfifo_put (&result_fifo, result);
        }
        else if (string_cmp (s_dev->kernel_buffer, "CT", 2)) // convert temperature
        {
            reset (onewire_pin);

            char data[2];
            data[0] = 0xCC;
            data[1] = 0x44;
            write_cmd (onewire_pin, data, 2);

            // give data to the client so signal command has finished
            struct read_data_t *result = kmalloc (sizeof (struct read_data_t), GFP_KERNEL);
            result->size = 1;
            result->data[0] = '-';
            kfifo_put (&result_fifo, result);
        }
        else // Set the value for the PIN
        {
            if (s_dev->kernel_buffer[0] == '0' || s_dev->kernel_buffer[0] == '1')
            {
                int value = s_dev->kernel_buffer[0] - '0';
                pr_info ("value %i\n", value);
                gpiod_direction_output (onewire_pin, value);
            }
        }
    }

    printk (KERN_INFO "%s: Wrote %zu bytes to device (offset: %lld)\n", MODULE_NAME, count, *f_pos);
    return bytes_written;
}

// File operations structure
static struct file_operations fops = {
    .open = onewire_open,
    .release = onewire_release,
    .read = onewire_read,
    .write = onewire_write,
    .owner = THIS_MODULE,
};

/**
 * Initialize the driver
 */
static int
onewire_probe (struct platform_device *pdev)
{
    pr_info ("onewire: onewire_probe");

    struct device *dev = &pdev->dev;
    const char *label;
    int ret;
    int err;

    // checking if the device haas the property label
    if (!device_property_present (dev, "label"))
    {
        pr_crit ("Device property 'label' not found!\n");
        return -1;
    }

    err = device_property_read_string (dev, "label", &label);
    if (err)
    {
        pr_crit ("dt_gpio - Error! Could not read 'label'\n");
        return -1;
    }
    pr_info ("dt_gpio - label: %s\n", label);

    // checking if the device haas the property onewire-gpios
    if (!device_property_present (dev, "onewire-gpios"))
    {
        pr_crit ("Device property 'onewire-gpios' not found!\n");
        return -1;
    }

    onewire_pin = gpiod_get (dev, PIN_ONEWIRE, GPIOD_OUT_HIGH);
    if (IS_ERR (onewire_pin))
    {
        pr_crit ("gpiod_get failed: %ld\n", PTR_ERR (onewire_pin));
        return PTR_ERR (onewire_pin);
    }
    gpiod_direction_output (onewire_pin, GPIOD_OUT_HIGH); // the the direction to input

    // initializing the character device
    s_dev = kmalloc (sizeof (struct onewire_dev), GFP_KERNEL);
    if (!s_dev)
    {
        ret = -ENOMEM;
        printk (KERN_ERR "%s: Failed to allocate device structure\n", MODULE_NAME);
        goto unregister_region;
    }
    memset (s_dev, 0, sizeof (struct onewire_dev));

    // Allocate kernel buffer
    s_dev->buffer_size = BUFFER_SIZE; // Use page size for buffer
    s_dev->kernel_buffer = kmalloc (s_dev->buffer_size, GFP_KERNEL);
    if (!s_dev->kernel_buffer)
    {
        ret = -ENOMEM;
        printk (KERN_ERR "%s: Failed to allocate kernel buffer\n", MODULE_NAME);
        goto free_device_struct;
    }
    memset (s_dev->kernel_buffer, 0, s_dev->buffer_size * sizeof (char));

    // initializing character device
    major_number = register_chrdev (0, MODULE_NAME, &fops);
    if (major_number < 0)
    {
        pr_alert ("Registering char device failed with %d\n", major_number);
        goto free_kernel_buffer;
    }

    // create the class
    cls = class_create (MODULE_NAME);
    if (IS_ERR (cls))
    {
        ret = -EINVAL;
        goto unregister_char;
    }

    device_create (cls, NULL, MKDEV (major_number, 0), NULL, MODULE_NAME);

    pr_info ("Device created on /dev/%s\n", MODULE_NAME);

    // initialize data structures
    INIT_KFIFO (result_fifo);

    device_opened = false;

    return 0;

unregister_char:
    unregister_chrdev (major_number, MODULE_NAME);
free_kernel_buffer:
    kfree (s_dev->kernel_buffer);
free_device_struct:
    kfree (s_dev);
unregister_region:
    gpiod_put (onewire_pin); // gree gppio

    return ret;
}

static int
onewire_remove (struct platform_device *pdev)
{
    pr_info ("onewire:  onewire_remove");

    kfree (s_dev->kernel_buffer);

    // free gpio pin
    gpiod_put (onewire_pin);

    // destroy character device
    device_destroy (cls, MKDEV (major_number, 0));
    class_destroy (cls);

    // unregister
    unregister_chrdev (major_number, MODULE_NAME);

    // free gpio pin
    pr_info ("good bye reader!\n");

    return 0;
}

static const struct of_device_id my_of_match[]
    = { { .compatible = "my-onewire" }, { /* sentinel */ } };
MODULE_DEVICE_TABLE (of, my_of_match);

struct platform_driver my_driver = {
    .driver = {
        .name = MODULE_NAME,
        .of_match_table = my_of_match,
    },
    .probe = onewire_probe,
    .remove = onewire_remove,
};

module_platform_driver (my_driver);

MODULE_LICENSE ("GPL");
