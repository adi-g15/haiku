/*
 * Copyright 2018 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		B Krishnan Iyer, krishnaniyer97@gmail.com
 */
#include "mmc_bus.h"


#define	MMC_BUS_DEVICE_NAME "bus_managers/mmc_bus/device/v1"


device_manager_info* gDeviceManager = NULL;


static status_t
mmc_bus_init(device_node* node, void** _device)
{
	CALLED();
	MMCBus* device = new(std::nothrow) MMCBus(node);
	if (device == NULL) {
		ERROR("Unable to allocate MMC bus\n");
		return B_NO_MEMORY;
	}

	status_t result = device->InitCheck();
	if (result != B_OK) {
		TRACE("failed to set up mmc bus device object\n");
		return result;
	}
	TRACE("MMC bus object created\n");

	*_device = device;
	return B_OK;
}


static void
mmc_bus_uninit(void* _device)
{
	CALLED();
	MMCBus* device = (MMCBus*)_device;
	delete device;
}


static status_t
mmc_bus_register_child(void* _device)
{
	CALLED();
	MMCBus* device = (MMCBus*)_device;
	device->Rescan();
	return B_OK;
}


static void
mmc_bus_removed(void* _device)
{
	CALLED();
}


status_t
mmc_bus_added_device(device_node* parent)
{
	CALLED();

	device_attr attributes[] = {
		{ B_DEVICE_BUS, B_STRING_TYPE, { string: "mmc"}},
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "MMC bus root"}},
		{ NULL }
	};

	return gDeviceManager->register_node(parent, MMC_BUS_DEVICE_NAME,
		attributes, NULL, NULL);
}


static status_t
mmc_bus_execute_command(device_node* node, uint8_t command, uint32_t argument,
	uint32_t* result)
{
	// FIXME store the parent cookie in the bus cookie or something instead of
	// getting/putting the parent each time.
	driver_module_info* mmc;
	void* cookie;

	TRACE("In mmc_bus_execute_command\n");
	device_node* parent = gDeviceManager->get_parent_node(node);
	gDeviceManager->get_driver(parent, &mmc, &cookie);
	gDeviceManager->put_node(parent);

	MMCBus* bus = (MMCBus*)cookie;

	bus->AcquireBus();
	status_t error = bus->ExecuteCommand(command, argument, result);
	bus->ReleaseBus();
	return error;
}


static status_t
mmc_bus_read_naive(device_node* node, uint16_t rca, off_t pos, void* buffer,
	size_t* _length)
{
	// FIXME store the parent cookie in the bus cookie or something instead of
	// getting/putting the parent each time.
	driver_module_info* mmc;
	void* cookie;

	device_node* parent = gDeviceManager->get_parent_node(node);
	gDeviceManager->get_driver(parent, &mmc, &cookie);
	gDeviceManager->put_node(parent);

	MMCBus* bus = (MMCBus*)cookie;
	bus->AcquireBus();
	status_t result = bus->Read(rca, pos, buffer, _length);
	bus->ReleaseBus();
	return result;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			// Nothing to do
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			break;
	}

	return B_ERROR;
}


driver_module_info mmc_bus_device_module = {
	{
		MMC_BUS_DEVICE_NAME,
		0,
		std_ops
	},
	NULL, // supported devices
	NULL, // register node
	mmc_bus_init,
	mmc_bus_uninit,
	mmc_bus_register_child,
	NULL, // rescan
	mmc_bus_removed,
	NULL, // suspend
	NULL // resume
};


mmc_device_interface mmc_bus_controller_module = {
	{
		{
			MMC_BUS_MODULE_NAME,
			0,
			&std_ops
		},

		NULL, // supported devices
		mmc_bus_added_device,
		NULL,
		NULL,
		NULL
	},
	mmc_bus_execute_command,
	mmc_bus_read_naive
};


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&gDeviceManager },
	{}
};


module_info* modules[] = {
	(module_info*)&mmc_bus_controller_module,
	(module_info*)&mmc_bus_device_module,
	NULL
};
