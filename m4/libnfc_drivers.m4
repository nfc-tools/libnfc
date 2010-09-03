dnl Handle drivers arguments list

AC_DEFUN([LIBNFC_ARG_WITH_DRIVERS],
[
  AC_MSG_CHECKING(which drivers to build)
  AC_ARG_WITH(drivers,
  AC_HELP_STRING([--with-drivers=DRIVERS], [Use a custom driver set, where DRIVERS is a coma-separated list of drivers to build support for. Available drivers are: 'acr122', 'arygon', 'pn531_usb', 'pn533_usb' and 'pn532_uart'. Default drivers set is 'acr122,arygon,pn531_usb,pn533_usb'. The special driver set 'all' compile all available drivers.]),
  [       case "${withval}" in
          yes | no)
                  dnl ignore calls without any arguments
                  DRIVER_BUILD_LIST="default"
                  AC_MSG_RESULT(default drivers)
                  ;;
          *)
                  DRIVER_BUILD_LIST=`echo ${withval} | sed "s/,/ /g"`
                  AC_MSG_RESULT(${DRIVER_BUILD_LIST})
                  ;;
          esac
  ],
  [
          DRIVER_BUILD_LIST="default"
          AC_MSG_RESULT(default drivers)
  ]
  )
  
  case "${DRIVER_BUILD_LIST}" in
    default)
                  DRIVER_BUILD_LIST="acr122 arygon pn531_usb pn533_usb"
                  ;;
    all)
                  DRIVER_BUILD_LIST="acr122 arygon pn531_usb pn533_usb pn532_uart"
                  ;;
  esac
  
  DRIVERS_CFLAGS=""

  driver_acr122_enabled="no"
  driver_pn531_usb_enabled="no"
  driver_pn533_usb_enabled="no"
  driver_arygon_enabled="no"
  driver_pn532_uart_enabled="no"

  for driver in ${DRIVER_BUILD_LIST}
  do
    case "${driver}" in
    acr122)
                  pcsc_required="yes"
		  driver_acr122_enabled="yes"
                  DRIVERS_CFLAGS="$DRIVERS_CFLAGS -DDRIVER_ACR122_ENABLED"
                  ;;
    pn531_usb)
                  libusb_required="yes"
		  driver_pn531_usb_enabled="yes"
                  DRIVERS_CFLAGS="$DRIVERS_CFLAGS -DDRIVER_PN531_USB_ENABLED"
                  ;;
    pn533_usb)
                  libusb_required="yes"
		  driver_pn533_usb_enabled="yes"
                  DRIVERS_CFLAGS="$DRIVERS_CFLAGS -DDRIVER_PN533_USB_ENABLED"
                  ;;
    arygon)
                  driver_arygon_enabled="yes"
                  DRIVERS_CFLAGS="$DRIVERS_CFLAGS -DDRIVER_ARYGON_ENABLED"
                  ;;
    pn532_uart)
                  driver_pn532_uart_enabled="yes"
                  DRIVERS_CFLAGS="$DRIVERS_CFLAGS -DDRIVER_PN532_UART_ENABLED"
                  ;;
    *)
                  AC_MSG_ERROR([Unknow driver: $driver])
                  ;;
    esac
  done
  AC_SUBST(DRIVERS_CFLAGS)
  AM_CONDITIONAL(DRIVER_ACR122_ENABLED, [test x"$driver_acr122_enabled" = xyes])
  AM_CONDITIONAL(DRIVER_PN531_USB_ENABLED, [test x"$driver_pn531_usb_enabled" = xyes])
  AM_CONDITIONAL(DRIVER_PN533_USB_ENABLED, [test x"$driver_pn533_usb_enabled" = xyes])
  AM_CONDITIONAL(DRIVER_ARYGON_ENABLED, [test x"$driver_arygon_enabled" = xyes])
  AM_CONDITIONAL(DRIVER_PN532_UART_ENABLED, [test x"$driver_pn532_uart_enabled" = xyes])
])

AC_DEFUN([LIBNFC_DRIVERS_SUMMARY],[
echo
echo "Selected drivers:"
echo "   acr122........... $driver_acr122_enabled"
echo "   arygon........... $driver_arygon_enabled"
echo "   pn531_usb........ $driver_pn531_usb_enabled"
echo "   pn532_uart....... $driver_pn532_uart_enabled"
echo "   pn533_usb........ $driver_pn533_usb_enabled"
])
