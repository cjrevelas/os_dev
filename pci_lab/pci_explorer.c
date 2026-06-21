#include <linux/module.h>
#include <linux/pci.h>

static int __init pci_explorer_init(void)
{
    struct pci_dev *pdev = NULL;

    pr_info("pci_explorer: module loaded\n");

    for_each_pci_dev(pdev) {

        pr_info(
            "PCI Device: %04x:%04x "
            "bus=%02x slot=%02x func=%02x irq=%u\n",
            pdev->vendor,
            pdev->device,
            pdev->bus->number,
            PCI_SLOT(pdev->devfn),
            PCI_FUNC(pdev->devfn),
            pdev->irq
        );
    }

    //
    // After iterating all devices let's find
    // one in particular
    //
    pdev = pci_get_device( 0x80ee, 0xcafe, NULL );
    if ( !pdev )
    {
        pr_info( "Device not found..\n" );
        return 0;
    }
    pr_info ( "name = %s\n", pci_name( pdev ) );

    return 0;
}

static void __exit pci_explorer_exit(void)
{
    pr_info("pci_explorer: module unloaded\n");
}

module_init(pci_explorer_init);
module_exit(pci_explorer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cjrevelas");
MODULE_DESCRIPTION("PCI Explorer");