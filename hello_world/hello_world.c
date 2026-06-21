#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/module.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/err.h>


dev_t dev = 0;
static struct class *dev_class;

//
// Module init function
//
// It allocates a major number for the device and
// prints information messages.
//
static int __init hello_world_init( void )
{
  printk( KERN_INFO "Welcome to linux driver development!\n" );

  //
  // Allocating major number dynamically
  //
  if ( alloc_chrdev_region( &dev, 0, 1, "cjr_dev" ) < 0 )
  {
    printk( KERN_INFO "Cannot allocate major number for device 1\n" );
    return -1;
  }
  printk( KERN_INFO "Major %d Minor %d \n", MAJOR(dev), MINOR(dev) );


  //
  // Creating the struct class
  //
  dev_class = class_create( "cjr_class" );
  if ( IS_ERR( dev_class ) )
  {
    printk( KERN_INFO "Cannot create the struct class for the device\n" );
    goto clean_class;
  }

  //
  // Creating the device
  //
  if ( IS_ERR( device_create( dev_class, NULL, dev, NULL, "cjr_device" ) ) )
  {
    printk( KERN_INFO "Cannot create the device\n" );
    goto clean_device;
  }
  printk( KERN_INFO "Kernel module inserted successfully.." );
  return 0;

clean_class:
  class_destroy( dev_class );
clean_device:
  unregister_chrdev_region( dev, 1 );
  return -1;
}

//
// Module exit function
//
static void __exit hello_world_exit ( void )
{
  device_destroy( dev_class, dev );
  class_destroy( dev_class );
  unregister_chrdev_region( dev, 1 );
  printk( KERN_INFO "Kernel module removed successfully..\n" );
}

module_init( hello_world_init );
module_exit( hello_world_exit );

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Constantinos J. Revelas");
MODULE_DESCRIPTION("A simple driver automatically creating a device file");
MODULE_VERSION("1.0.1");
