#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/module.h>

//
// Module init function
//
static int __init hello_world_init( void )
{
  printk( KERN_INFO "Welcome to linux driver development\n" );
  return 0;
}

//
// Module exit function
//
static void __exit hello_world_exit ( void )
{
  printk( KERN_INFO "Kernel module removed succesfully\n" );
}

module_init( hello_world_init );
module_exit( hello_world_exit );

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Constantinos J. Revelas");
MODULE_DESCRIPTION("A simple hello world driver");
MODULE_VERSION("1.0.0");
