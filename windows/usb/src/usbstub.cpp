/**************************************************************************
 *
 *  Copyright 2010, Roger Brown
 *
 *  This file is part of Roger Brown's Toolkit.
 *
 *  Roger Brown's Toolkit is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Roger Brown's Toolkit is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Roger Brown's Toolkit.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/* 
 * $Id: usbstub.cpp 1220 2010-05-04 03:14:56Z roger.brown $
 */

/*
 * this is a stub loader for the LIBUSB0.DLL
 */

#include <usb.h>

#define LIBUSB_CALLTYPE		__cdecl

extern "C"
{
	typedef void (LIBUSB_CALLTYPE *voidProc)(void);
}

class CStubLoader
{
	HMODULE hDll;
	CRITICAL_SECTION cs;
	const char *name;

	void bomb(DWORD dw)
	{
		RaiseException(dw,EXCEPTION_NONCONTINUABLE,0,0);
	}

public:
	~CStubLoader()
	{
/*		if (hDll)
		{
			FreeLibrary(hDll);
		}*/

		DeleteCriticalSection(&cs);
	}

	CStubLoader(const char *n) : name(n)
	{
		InitializeCriticalSection(&cs);
	}

	HMODULE get_dll(void)
	{
		HMODULE h=NULL;
		DWORD dw=0;

		EnterCriticalSection(&cs);

		if (!hDll)
		{
			hDll=LoadLibrary(name);
			if (!hDll) dw=GetLastError();
		}

		h=hDll;

		LeaveCriticalSection(&cs);

		if (!h)
		{
			bomb(dw);
		}

		return h;
	}

	voidProc GetProc(const char *name)
	{
		voidProc p=(voidProc)GetProcAddress(get_dll(),name);

		if (!p) 
		{
			bomb(GetLastError());
		}

		return p;
	}
};


static CStubLoader libusb0("LIBUSB0");

extern "C"
{
#	define MAP_FN(ret,name,args)			\
		typedef ret (LIBUSB_CALLTYPE *pfn_##name##_t)args;					\
		static ret LIBUSB_CALLTYPE load_##name args;						\
		static pfn_##name##_t pfn_##name=load_##name,test_##name=##name;

#	define LOAD_FN(name)		int success=1; __try { pfn_##name=(pfn_##name##_t)libusb0.GetProc(#name); } __except(1) { success=0; }

	MAP_FN(int,usb_reset,(usb_dev_handle *dev))
	MAP_FN(int,usb_claim_interface,(usb_dev_handle *dev,int))
	MAP_FN(int,usb_find_busses,(void))
	MAP_FN(int,usb_find_devices,(void))
	MAP_FN(void,usb_init,(void))
	MAP_FN(int,usb_close,(usb_dev_handle *dev))
	MAP_FN(int,usb_bulk_write,(usb_dev_handle *,int,char *,int,int));
	MAP_FN(int,usb_bulk_read,(usb_dev_handle *,int,char *,int,int));
	MAP_FN(usb_dev_handle *,usb_open,(struct usb_device *));
	MAP_FN(int,usb_set_configuration,(usb_dev_handle *,int));
	MAP_FN(usb_bus *,usb_get_busses,(void));
	MAP_FN(int,usb_release_interface,(usb_dev_handle *,int));
	MAP_FN(int,usb_get_string_simple,(usb_dev_handle *dev, int index, char *buf,size_t buflen));

	static int LIBUSB_CALLTYPE load_usb_reset(usb_dev_handle *dev)
	{
		LOAD_FN(usb_reset);

		return success ? usb_reset(dev) : -1;
	}

	static int LIBUSB_CALLTYPE load_usb_claim_interface(usb_dev_handle *dev,int interface)
	{
		LOAD_FN(usb_claim_interface)

		return success ? usb_claim_interface(dev,interface) : -1;
	}

	static int LIBUSB_CALLTYPE load_usb_release_interface(usb_dev_handle *dev,int interface)
	{
		LOAD_FN(usb_release_interface)

		return success ? usb_release_interface(dev,interface) : -1;
	}

	static int LIBUSB_CALLTYPE load_usb_close(usb_dev_handle *dev)
	{
		LOAD_FN(usb_close)

		return success ? usb_close(dev) : -1;
	}

	static usb_dev_handle * LIBUSB_CALLTYPE load_usb_open(struct usb_device *dev)
	{
		LOAD_FN(usb_open)

		return success ? usb_open(dev) : NULL;
	}

	static int LIBUSB_CALLTYPE load_usb_find_devices(void)
	{
		LOAD_FN(usb_find_devices)

		return success ? usb_find_devices() : -1;
	}

	static int LIBUSB_CALLTYPE load_usb_find_busses(void)
	{
		LOAD_FN(usb_find_busses)

		return success ? usb_find_busses() : -1;
	}

	static int LIBUSB_CALLTYPE load_usb_set_configuration(usb_dev_handle *dev,int configuration)
	{
		LOAD_FN(usb_set_configuration)

		return success ? usb_set_configuration(dev,configuration) : -1;
	}

	static usb_bus * LIBUSB_CALLTYPE load_usb_get_busses(void)
	{
		LOAD_FN(usb_get_busses)

		return success ? usb_get_busses() : NULL;
	}

	static void LIBUSB_CALLTYPE load_usb_init(void)
	{
		LOAD_FN(usb_init)

		if (success) usb_init();
	}

	static int LIBUSB_CALLTYPE load_usb_bulk_read(usb_dev_handle *dev,int ep,char *bytes,int size,int timeout)
	{
		LOAD_FN(usb_bulk_read);

		return success ? usb_bulk_read(dev,ep,bytes,size,timeout) : -1;
	}

	static int LIBUSB_CALLTYPE load_usb_bulk_write(usb_dev_handle *dev,int ep,char *bytes,int size,int timeout)
	{
		LOAD_FN(usb_bulk_write);

		return success ? usb_bulk_write(dev,ep,bytes,size,timeout) : -1;
	}

	static int LIBUSB_CALLTYPE load_usb_get_string_simple(usb_dev_handle *dev, int index, char *buf,
                            size_t buflen)
	{
		LOAD_FN(usb_get_string_simple);

		return success ? usb_get_string_simple(dev,index,buf,buflen) : -1;
	}
}

int LIBUSB_CALLTYPE usb_claim_interface(usb_dev_handle *dev, int interface)
{
	return pfn_usb_claim_interface(dev,interface);
}

int LIBUSB_CALLTYPE usb_reset(usb_dev_handle *dev)
{
	return pfn_usb_reset(dev);
}

int LIBUSB_CALLTYPE usb_find_busses(void)
{
	return pfn_usb_find_busses();
}

int LIBUSB_CALLTYPE usb_find_devices(void)
{
	return pfn_usb_find_devices();
}

void LIBUSB_CALLTYPE usb_init(void)
{
	pfn_usb_init();
}

int LIBUSB_CALLTYPE usb_close(usb_dev_handle *dev)
{
	return pfn_usb_close(dev);
}

usb_dev_handle * LIBUSB_CALLTYPE usb_open(struct usb_device *dev)
{
	return pfn_usb_open(dev);
}

int LIBUSB_CALLTYPE usb_set_configuration(usb_dev_handle *dev, int configuration)
{
	return pfn_usb_set_configuration(dev,configuration);
}

struct usb_bus * LIBUSB_CALLTYPE usb_get_busses(void)
{
	return pfn_usb_get_busses();
}

int LIBUSB_CALLTYPE usb_release_interface(usb_dev_handle *dev, int interface)
{
	return pfn_usb_release_interface(dev,interface);
}

int LIBUSB_CALLTYPE usb_bulk_write(usb_dev_handle *dev, int ep, char *bytes, int size,
                 int timeout)
{
	return pfn_usb_bulk_write(dev,ep,bytes,size,timeout);
}

int LIBUSB_CALLTYPE usb_bulk_read(usb_dev_handle *dev, int ep, char *bytes, int size,
                int timeout)
{
	return pfn_usb_bulk_read(dev,ep,bytes,size,timeout);
}

int LIBUSB_CALLTYPE usb_get_string_simple(usb_dev_handle *dev, int index, char *buf,
                            size_t buflen)
{
	return pfn_usb_get_string_simple(dev,index,buf,buflen);
}





