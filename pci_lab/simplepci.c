#include <linux/module.h>
#include <linux/pci.h>

#define MY_VENDOR_ID 0x1234
#define MY_DEVICE_ID 0x11e8


static int my_probe( struct pci_dev *pdev, const struct pci_device_id *id )
{
  pr_info( "simplepci: probe called\n" );

  pr_info( "simplepci: vendor=%04x device=%04x\n",
            pdev->vendor,
            pdev->device );

  pr_info( "simplepci: irq=%u\n", pdev->irq );

  if ( pci_enable_device( pdev ) )
  {
    pr_err( "simplepci: pci_enable_device failed\n" );
  }

  pci_set_master( pdev );

  for ( int i=0; i<PCI_STD_NUM_BARS; ++i )
  {
    resource_size_t start;
    resource_size_t len;
    unsigned long flags;

    start = pci_resource_start( pdev, i );
    len   = pci_resource_len( pdev, i );
    flags = pci_resource_flags( pdev, i );

    pr_info( "simplepci: BAR%d start=0x%llx len=0x%llx flags=0x%lx\n",
              i,
              (unsigned long long int)start,
              (unsigned long long int)len,
              flags );
  }

  return 0;
}

static void my_remove( struct pci_dev *pdev )
{
  pr_info( "simplepci: remove called\n" );

  pci_disable_device( pdev );
}

static const struct pci_device_id my_pci_ids[] = {
  { PCI_DEVICE( MY_VENDOR_ID, MY_DEVICE_ID ) },
  { 0, } 
};

MODULE_DEVICE_TABLE( pci, my_pci_ids );

static struct pci_driver my_driver = {
  .name     = "simplepci",
  .id_table = my_pci_ids,
  .probe    = my_probe,
  .remove   = my_remove
};

module_pci_driver( my_driver );

MODULE_LICENSE( "GPL" );
MODULE_AUTHOR( "CJREVELAS" );
MODULE_DESCRIPTION( "PCI Driver Learning Lab" );


