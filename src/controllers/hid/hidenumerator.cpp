#include "controllers/hid/hidenumerator.h"

#include <hidapi.h>

#if defined(__APPLE__) && HID_API_VERSION >= HID_API_MAKE_VERSION(0, 12, 0)
#include <hidapi_darwin.h>
#endif

#include "controllers/hid/hidcontroller.h"
#include "controllers/hid/hiddenylist.h"
#include "controllers/hid/hidusagetables.h"
#include "moc_hidenumerator.cpp"
#include "util/cmdlineargs.h"

namespace mixxx {

namespace hid {

constexpr unsigned short kGenericDesktopUsagePage = 0x01;

constexpr unsigned short kGenericDesktopMouseUsage = 0x02;
constexpr unsigned short kGenericDesktopKeyboardUsage = 0x06;

// Apple has two two different vendor IDs which are used for different devices.
constexpr unsigned short kAppleVendorId = 0x5ac;
constexpr unsigned short kAppleIncVendorId = 0x004c;

} // namespace hid

} // namespace mixxx

bool HidEnumerator::recognizeDevice(const hid_device_info& device_info) const {
    // Skip mice and keyboards. Users can accidentally disable their mouse
    // and/or keyboard by enabling them as HID controllers in Mixxx.
    // https://github.com/mixxxdj/mixxx/issues/10498
    if (!CmdlineArgs::Instance().getDeveloper() &&
            device_info.usage_page == mixxx::hid::kGenericDesktopUsagePage &&
            (device_info.usage == mixxx::hid::kGenericDesktopMouseUsage ||
                    device_info.usage == mixxx::hid::kGenericDesktopKeyboardUsage)) {
        return false;
    }

    // Apple includes a variety of HID devices in their computers, not all of which
    // match the filter above for keyboards and mice, for example "Magic Trackpad",
    // "Internal Keyboard", and "T1 Controller". Apple is likely to keep changing
    // these devices in future computers and none of these devices are DJ controllers,
    // so skip all Apple HID devices rather than maintaining a list of specific devices
    // to skip.
    if (device_info.vendor_id == mixxx::hid::kAppleVendorId
          || device_info.vendor_id == mixxx::hid::kAppleIncVendorId) {
        return false;
    }

    // Exclude specific devices from the denylist.
    for (const hid_denylist_t& denylisted : hid_denylisted) {
        // If vendor ids are specified and do not match, skip.
        if (denylisted.vendor_id != kAnyValue &&
                device_info.vendor_id != denylisted.vendor_id) {
            continue;
        }
        // If product IDs are specified and do not match, skip.
        if (denylisted.product_id != kAnyValue &&
                device_info.product_id != denylisted.product_id) {
            continue;
        }
        // Denylist entry based on interface number
        // If interface number is present and the interface numbers do not
        // match, skip.
        if (denylisted.interface_number != kInvalidInterfaceNumber &&
                device_info.interface_number != denylisted.interface_number) {
            continue;
        }
        // Denylist entry based on usage_page and usage (both required)
        if (denylisted.usage_page != kAnyValue && denylisted.usage != kAnyValue) {
            // If usage_page is different, skip.
            if (device_info.usage_page != denylisted.usage_page) {
                continue;
            }
            // If usage is different, skip.
            if (device_info.usage != denylisted.usage) {
                continue;
            }
        }
        return false;
    }
    return true;
}

HidEnumerator::~HidEnumerator() {
    qDebug() << "Deleting HID devices...";
    while (m_devices.size() > 0) {
        delete m_devices.takeLast();
    }
    hid_exit();
}

const char *hid_bus_name(hid_bus_type bus_type) {
	static const char *const HidBusTypeName[] = {
		"Unknown",
		"USB",
		"Bluetooth",
		"I2C",
		"SPI",
	};

	if ((int)bus_type < 0)
		bus_type = HID_API_BUS_UNKNOWN;
	if ((int)bus_type >= (int)(sizeof(HidBusTypeName) / sizeof(HidBusTypeName[0])))
		bus_type = HID_API_BUS_UNKNOWN;

	return HidBusTypeName[bus_type];
}

void print_device(struct hid_device_info *cur_dev) {
	printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls", cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
	printf("\n");
	printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
	printf("  Product:      %ls\n", cur_dev->product_string);
	printf("  Release:      %hx\n", cur_dev->release_number);
	printf("  Interface:    %d\n",  cur_dev->interface_number);
	printf("  Usage (page): 0x%hx (0x%hx)\n", cur_dev->usage, cur_dev->usage_page);
	printf("  Bus type: %u (%s)\n", (unsigned)cur_dev->bus_type, hid_bus_name(cur_dev->bus_type));
	printf("\n");
}

void print_devices_with_descriptor(struct hid_device_info *cur_dev) {
	for (; cur_dev; cur_dev = cur_dev->next) {
		print_device(cur_dev);
	}
}

QList<Controller*> HidEnumerator::queryDevices() {

    if (hid_init()) {
        qWarning() << "Failed to initialize HIDAPI";
        return m_devices;
    }

    #if defined(__APPLE__) && HID_API_VERSION >= HID_API_MAKE_VERSION(0, 12, 0)
	// To work properly needs to be called before hid_open/hid_open_path after hid_init.
	hid_darwin_set_open_exclusive(0);
    #endif

    qInfo() << "Scanning USB HID devices";

    QStringList enumeratedDevices;
    hid_device_info* device_info_list = hid_enumerate(0x0, 0x0);
    print_devices_with_descriptor(device_info_list);
    // for (const auto* device_info = device_info_list;
    //         device_info;
    //         device_info = device_info->next) {
    //     auto deviceInfo = mixxx::hid::DeviceInfo(*device_info);
    //     // The hidraw backend of hidapi on Linux returns many duplicate hid_device_info's from hid_enumerate,
    //     // so filter them out.
    //     // https://github.com/libusb/hidapi/issues/298
    //     if (enumeratedDevices.contains(deviceInfo.pathRaw())) {
    //         qInfo() << "Duplicate HID device, excluding" << deviceInfo;
    //         continue;
    //     }
    //     if (device_info->bus_type != HID_API_BUS_USB) {
    //         qInfo() << "Excluding non-USB HID device" << deviceInfo;
    //         continue;
    //     }
    //     enumeratedDevices.append(QString(deviceInfo.pathRaw()));

    //     if (!recognizeDevice(*device_info)) {
    //         qInfo()
    //                 << "Excluding HID device"
    //                 << deviceInfo;
    //         continue;
    //     }
    //     qInfo() << "Found HID device:"
    //             << deviceInfo;

    //     if (!deviceInfo.isValid()) {
    //         qWarning() << "HID device permissions problem or device error."
    //                    << "Your account needs write access to HID controllers.";
    //         continue;
    //     }

    //     HidController* newDevice = new HidController(std::move(deviceInfo));
    //     m_devices.push_back(newDevice);
    // }
    hid_free_enumeration(device_info_list);

    return m_devices;
}
