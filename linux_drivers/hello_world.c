#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/module.h>


// We will allocate a major number dynamically
dev_t dev = 0;

//
// Module init function
//
// It allocates a major number for the device and
// prints information messages.
//
static int __init hello_world_init( void )
{
  printk( KERN_INFO "Welcome to linux driver development\n" );


  if ( alloc_chrdev_region( &dev, 0, 1, "cjr_dev" ) < 0 )
  {
    printk( KERN_INFO "Cannot allocate major number for device 1\n" );
    return -1;
  }

  printk( KERN_INFO "Major %d Minor %d \n", MAJOR(dev), MINOR(dev) );
  printk( KERN_INFO "Kernel module inserted successfully.." );

  return 0;
}

//
// Module exit function
//
static void __exit hello_world_exit ( void )
{
  printk( KERN_INFO "Kernel module removed successfully..\n" );
}

module_init( hello_world_init );
module_exit( hello_world_exit );

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Constantinos J. Revelas");
MODULE_DESCRIPTION("A simple driver allocating dynamically the major and minor number");
MODULE_VERSION("1.0.1");