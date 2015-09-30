platform.













struct platform_device {
	const char	* name;
	int		id;
	bool		id_auto;
	struct device	dev;
	u32		num_resources;
	struct resource	* resource;

	const struct platform_device_id	*id_entry;

	/* MFD cell pointer */
	struct mfd_cell *mfd_cell;

	/* arch specific additions */
	struct pdev_archdata	archdata;
};

struct device {
	struct device		*parent;

	struct device_private	*p;

	struct kobject kobj;
	const char		*init_name; /* initial name of the device */
	const struct device_type *type;

	struct mutex		mutex;	/* mutex to synchronize calls to
					 * its driver.
					 */

	struct bus_type	*bus;		/* type of bus device is on */
	struct device_driver *driver;	/* which driver has allocated this
					   device */
	void		*platform_data;	/* Platform specific data, device
					   core doesn't touch it */
	struct dev_pm_info	power;
	struct dev_pm_domain	*pm_domain;

#ifdef CONFIG_NUMA
	int		numa_node;	/* NUMA node this device is close to */
#endif
	u64		*dma_mask;	/* dma mask (if dma'able device) */
	u64		coherent_dma_mask;/* Like dma_mask, but for
					     alloc_coherent mappings as
					     not all hardware supports
					     64 bit addresses for consistent
					     allocations such descriptors. */

	struct device_dma_parameters *dma_parms;

	struct list_head	dma_pools;	/* dma pools (if dma'ble) */

	struct dma_coherent_mem	*dma_mem; /* internal for coherent mem
					     override */
#ifdef CONFIG_CMA
	struct cma *cma_area;		/* contiguous memory area for dma
					   allocations */
#endif
	/* arch specific additions */
	struct dev_archdata	archdata;

	struct device_node	*of_node; /* associated device tree node */
	struct acpi_dev_node	acpi_node; /* associated ACPI device node */

	dev_t			devt;	/* dev_t, creates the sysfs "dev" */
	u32			id;	/* device instance */

	spinlock_t		devres_lock;
	struct list_head	devres_head;

	struct klist_node	knode_class;
	struct class		*class;
	const struct attribute_group **groups;	/* optional groups */

	void	(*release)(struct device *dev);
	struct iommu_group	*iommu_group;
};

struct resource {
	resource_size_t start;
	resource_size_t end;
	const char *name;
	unsigned long flags;
	struct resource *parent, *sibling, *child;
};

struct platform_device_id {
	char name[PLATFORM_NAME_SIZE];
	kernel_ulong_t driver_data
			__attribute__((aligned(sizeof(kernel_ulong_t))));
};

struct mfd_cell {
	const char		*name;
	int			id;

	/* refcounting for multiple drivers to use a single cell */
	atomic_t		*usage_count;
	int			(*enable)(struct platform_device *dev);
	int			(*disable)(struct platform_device *dev);

	int			(*suspend)(struct platform_device *dev);
	int			(*resume)(struct platform_device *dev);

	/* platform data passed to the sub devices drivers */
	void			*platform_data;
	size_t			pdata_size;
	/*
	 * Device Tree compatible string
	 * See: Documentation/devicetree/usage-model.txt Chapter 2.2 for details
	 */
	const char		*of_compatible;

	/*
	 * These resources can be specified relative to the parent device.
	 * For accessing hardware you should use resources from the platform dev
	 */
	int			num_resources;
	const struct resource	*resources;

	/* don't check for resource conflicts */
	bool			ignore_resource_conflicts;

	/*
	 * Disable runtime PM callbacks for this subdevice - see
	 * pm_runtime_no_callbacks().
	 */
	bool			pm_runtime_no_callbacks;
};

struct device_private {
	struct klist klist_children;
	struct klist_node knode_parent;
	struct klist_node knode_driver;
	struct klist_node knode_bus;
	struct list_head deferred_probe;
	void *driver_data;
	struct device *device;
};

struct kobject {
	const char		*name;
	struct list_head	entry;
	struct kobject		*parent;
	struct kset		*kset;
	struct kobj_type	*ktype;
	struct sysfs_dirent	*sd;
	struct kref		kref;
	unsigned int state_initialized:1;
	unsigned int state_in_sysfs:1;
	unsigned int state_add_uevent_sent:1;
	unsigned int state_remove_uevent_sent:1;
	unsigned int uevent_suppress:1;
};
