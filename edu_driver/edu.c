/*
* Linux-kernel headers.
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/string.h>

#define DRIVER_NAME "edu"

/*
* QEMU EDU device.
*/
#define EDU_VENDOR_ID 0x1234
#define EDU_DEVICE_ID 0x11e8

/*
* Define EDU BAR0 register offsets.
*/
#define EDU_REG_IDENT    0x00
#define EDU_REG_LIVENESS 0x04

/*
* Define EDU DMA register.
*/
#define EDU_REG_DMA_SRC   0x80
#define EDU_REG_DMA_DST   0x88
#define EDU_REG_DMA_COUNT 0x90
#define EDU_REG_DMA_CMD   0x98

/*
* EDU internal DMA buffer.
*/
#define EDU_DMA_BUFFER 0x40000

/*
* DMA buffer size.
*/
#define EDU_DMA_SIZE 4096

/*
* DMA command bits.
*/
#define EDU_DMA_START    0x01
#define EDU_DMA_FROM_EDU 0x02

/*
* Driver private data.
*/
struct edu_device {
    struct pci_dev *pdev;

    /*
    * BAR0 mapped into kernel virtual address space.
    * or
    * Kernel virtual address corresponding to BAR0.
    *
    */
    void __iomem *mmio;

    /*
    * DMA buffer
    *
    * dma_virt:
    *       CPU virtual address used by the CPU.
    *
    * dma_handle:
    *       DMA/bus address used by the device.
    */
    void *dma_virt;
    dma_addr_t dma_handle;
};


/*
* Define the PCI device ID table.
*/
static const struct pci_device_id edu_ids[] = {
    { PCI_DEVICE( EDU_VENDOR_ID, EDU_DEVICE_ID ) },
    {}
};

MODULE_DEVICE_TABLE( pci, edu_ids );

/*
* Perform one DMA transfer and wait for completion.
*/
static int edu_dma_transfer( struct edu_device *edev,
                             dma_addr_t src,
                             dma_addr_t dst,
                             u32 count,
                             u32 command)
{
    unsigned int timeout = 1000000;

    /*
    * Specify DMA source address.
    */
    writeq( src, edev->mmio + EDU_REG_DMA_SRC );

    /*
    * Specify DMA destination address.
    */
    writeq( dst, edev->mmio + EDU_REG_DMA_DST );

    /*
    * Specify number of bytes to transfer.
    */
    writel( count, edev->mmio + EDU_REG_DMA_COUNT );

    /*
    * Start DMA transfer: bit 0 = start.
    */
    writel( command, edev->mmio + EDU_REG_DMA_CMD );

    /*
    * Wait until the EDU device clears the START bit.
    */
    while ( readl( edev->mmio + EDU_REG_DMA_CMD ) & EDU_DMA_START )
    {
        cpu_relax();

        /*
        * Prevent an infinite loop if something goes wrong.
        */
        if ( --timeout == 0 )
        {
            pr_err( "edu: DMA timeout\n" );
            return -ETIMEDOUT;
        }

        /*
        * Avoid spinning completely flat-out.
        */
        udelay(1);
    }

    return 0;
}

/*
* edu_probe: called by the PCI core when a matching device is found.
*/
static int edu_probe( struct pci_dev *pdev, const struct pci_device_id *id )
{
    struct edu_device *edev;
    int ret;
    resource_size_t bar_start;
    resource_size_t bar_len;
    unsigned long bar_flags;

    /*
    * Test-data for the DMA transfer.
    */
    const char test_data[] = "Hello from DMA!";

    pr_info( "edu: probe() called\n" );

    /*
    * Allocate driver private data.
    *
    * IMPORTANT:
    * We use kzalloc(), therefore we must explicitly use kfree()
    * for deallocation.
    */
    edev = kzalloc( sizeof(*edev), GFP_KERNEL );
    if ( !edev )
        return -ENOMEM;

    edev->pdev = pdev;

    /*
    * Associate our private structure with this PCI device.
    */
    pci_set_drvdata( pdev, edev );

    /*
    * Enable the PCI device.
    */
    ret = pci_enable_device( pdev );
    if ( ret )
    {
        pr_err( "edu: pci_enable_device() failed\n" );
        goto err_free;
    }

    /*
    * Retrieve information about BAR0.
    */
    bar_start = pci_resource_start( pdev, 0 );
    bar_len   = pci_resource_len( pdev, 0 );
    bar_flags = pci_resource_flags( pdev, 0 );

    pr_info( "edu: BAR0 start = 0x%llx\n", (unsigned long long)bar_start );
    pr_info( "edu: BAR0 length = 0x%llx\n", (unsigned long long)bar_len );
    pr_info( "edu: BAR0 flags = 0x%lx\n", bar_flags );

    /*
    * Request ownership of the PCI resources.
    */
    ret = pci_request_regions( pdev, DRIVER_NAME );
    if ( ret )
    {
        pr_err( "edu: pci_request_regions() failed\n" );
        goto err_disable;
    }

    /*
    * Map BAR0 into kernel virtual address space.
    */
    edev->mmio = pci_iomap( pdev, 0, 0 );
    if ( !edev->mmio )
    {
        pr_err( "edu: pci_iomap() failed\n" );
        ret = -ENOMEM;
        goto err_release_regions;
    } 
    pr_info("edu: BAR0 mapped at %p\n", edev->mmio);

    /*
    * -------------------------------------------------------
    * MMIO TEST
    * -------------------------------------------------------
    */
    pr_info( "edu: IDENT    = 0x%08x\n", readl( edev->mmio + EDU_REG_IDENT ) );
    pr_info( "edu: LIVENESS = 0x%08x\n", readl( edev->mmio + EDU_REG_LIVENESS ) );

    /*
    * -------------------------------------------------------
    * DMA ADDRESS MASK
    * -------------------------------------------------------
    *
    * QEMU EDU device supports 28-bit DMA addresses by default.
    *
    * Therefore the device can address:
    *
    *          0x00000000 - 0x0ffffffff
    *
    */
    ret = dma_set_mask_and_coherent( &pdev->dev, DMA_BIT_MASK( 28 ) );
    
    if ( ret )
    {
        pr_err( "edu: failed to set 28-bit DMA mask\n" );
        goto err_iounmap;
    }

    pr_info( "edu: 28-bit DMA mask configured\n" );

    /*
    * -------------------------------------------------------
    * ALLOCATE DMA BUFFER
    * -------------------------------------------------------
    */
    edev->dma_virt = dma_alloc_coherent( &pdev->dev, EDU_DMA_SIZE, &edev->dma_handle, GFP_KERNEL );

    if ( !edev->dma_virt )
    {
        pr_err( "edu: dma_alloc_coherent() failed\n" );
        ret = -ENOMEM;
        goto err_iounmap;
    }

    pr_info( "edu: DMA CPU address = %px\n", edev->dma_virt );
    pr_info( "edu: DMA DEVICE address = 0x%llx\n", (unsigned long long)edev->dma_handle );
    pr_info( "edu: DMA DEVICE address = %pad\n", &edev->dma_handle );


    /*
    *--------------------------------------------------------
    * BUS MASTERING    
    *--------------------------------------------------------
    *
    * This enables the device to initiate PCI transactions.
    *
    * It does NOT perform DMA by itself.
    */
    pci_set_master( pdev );
    pr_info( "edu: PCI bus mastering enabled\n" );

    /*
    * Verify bus master enable.
    */
    {
        u16 command;

        ret = pci_read_config_word( pdev, PCI_COMMAND, &command );
        if ( ret )
        {
            pr_err( "edu: failed to read PCI command\n " );
            goto err_iounmap;
        }
        pr_info( "edu: PCI_COMMAND = 0x%04x\n", command );

        if ( command & PCI_COMMAND_MASTER )
        {
            pr_info( "edu: Bus master enable = 1\n" );
        }
        else
        {
            pr_err( "edu: Bus master enable = 0\n" );
            ret = -EIO;
            goto err_free_dma;
        }
    }

    /*
    *--------------------------------------------------------
    * PREPARE DMA BUFFER
    *--------------------------------------------------------
    *
    * CPU writes the test data into RAM.
    */
    memset( edev->dma_virt, 0, EDU_DMA_SIZE );
    memcpy( edev->dma_virt, test_data, sizeof( test_data ) );
    pr_info( "edu: DMA buffer before transfer: \"%s\"\n", (char *)edev->dma_virt );

    pr_info( "edu: device initialized successfully\n" );

    return 0;

/*
*--------------------------------------------------------
* ERROR PATHS
*--------------------------------------------------------
*/
err_free_dma:
    /*
    * Stop device from initiating PCI transactions.
    */
    pci_clear_master( pdev );

    /*
    * Free coherent DMA memory.
    */
    dma_free_coherent( &pdev->dev, EDU_DMA_SIZE, edev->dma_virt, edev->dma_handle );

err_iounmap:
    pci_iounmap( pdev, edev->mmio );

err_release_regions:
    pci_release_regions( pdev );

err_disable:
    pci_disable_device( pdev );

err_free:
    pci_set_drvdata( pdev, NULL );

    /*
    * We used kzalloc(), so we must explicitly free memory
    */
    kfree( edev );

    return ret;
}


/*
* edu_remove: called by the PCI core when the device is removed
*             or the driver is unloaded
*/
static void edu_remove( struct pci_dev *pdev )
{
    struct edu_device *edev;

    pr_info( "edu: remove() called\n" );

    edev = pci_get_drvdata( pdev );
    if ( !edev ) return;

    /*
    * Disable bus mastering before tearing the device down
    */
    pci_clear_master( pdev );
    pr_info( "edu: PCI bus mastering disabled\n" );

    /*
    * Free DMA memory
    */
    if ( edev->dma_virt )
    {
        dma_free_coherent( &pdev->dev, EDU_DMA_SIZE, edev->dma_virt, edev->dma_handle );
        pr_info( "edu: DMA buffer freed\n" );
    }

    /*
    * Unmap BAR0
    */
    if ( edev->mmio ) pci_iounmap( pdev, edev->mmio );

    /*
    * Disable the PCI device
    */
    pci_disable_device( pdev );

    /*
    * Release PCI resources
    */
    pci_release_regions( pdev );

    /*
    * Remove driver-private data
    */
    pci_set_drvdata( pdev, NULL );

    /*
    * We used kzalloc(), so we must explicitly free memory
    */
    kfree( edev );

    pr_info( "edu: device removed\n" );
}


/*
* Register the PCI driver
*/
static struct pci_driver edu_driver = {
    .name = DRIVER_NAME,
    .id_table = edu_ids,
    .probe = edu_probe,
    .remove = edu_remove
};


/*
* edu_init
*/
static int __init edu_init( void )
{
    pr_info( "edu: driver loading..\n" );

    return pci_register_driver( &edu_driver );
}


/*
* edu_exit
*/
static void __exit edu_exit( void )
{
    pr_info( "edu: driver unloading..\n" );

    pci_unregister_driver( &edu_driver );
}


module_init( edu_init );
module_exit( edu_exit );

MODULE_LICENSE( "GPL" );
MODULE_AUTHOR( "Constantinos J. Revelas" );
MODULE_DESCRIPTION( "QEMU EDU PCI Driver" );