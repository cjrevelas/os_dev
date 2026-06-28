#include <linux/module.h>
#include <linux/pci.h>


static const struct pci_device_id edu_ids[] = {
  { PCI_DEVICE( 0x1234, 0x11e8 ) },
  { } 
};

MODULE_DEVICE_TABLE( pci, edu_ids );


static int edu_probe( struct pci_dev *pdev, const struct pci_device_id *id )
{
  int ret;

  pr_info( "edu: probe() called\n" );

  ret = pci_enable_device( pdev );
  if ( ret )
  {
    pr_err ( "edu: pci_enable_device failed\n" );
    return ret;
  }

  pr_info( "edu_device enabled\n" );

  return 0;
}

static void edu_remove( struct pci_dev *pdev )
{
  pr_info( "edu: remove() called\n" );
}


static struct pci_driver edu_driver = {
  .name     = "edu",
  .id_table = edu_ids,
  .probe    = edu_probe,
  .remove   = edu_remove,
};

module_pci_driver( edu_driver );

MODULE_LICENSE( "GPL" );
MODULE_AUTHOR( "cjrevelas" );
MODULE_DESCRIPTION( "qemu edu pci driver" );