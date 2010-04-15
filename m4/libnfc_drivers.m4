dnl Handle drivers arguments list

AC_DEFUN([LIBNFC_ARG_WITH_DRIVERS],
[
  AC_MSG_CHECKING(which drivers to build)
  AC_ARG_WITH(drivers,
  AC_HELP_STRING([[[[--with-drivers=driver@<:@,driver...@:>@]]]], [Only use specific drivers (default set)]),
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

  for driver in ${DRIVER_BUILD_LIST}
  do
    case "${driver}" in
    acr122)
                  pcsc_required="yes"
                  DRIVERS_CFLAGS="$DRIVERS_CFLAGS -DDRIVER_ACR122_ENABLED"
                  ;;
    pn531_usb)
                  libusb_required="yes"
                  DRIVERS_CFLAGS="$DRIVERS_CFLAGS -DDRIVER_PN531_USB_ENABLED"
                  ;;
    pn533_usb)
                  libusb_required="yes"
                  DRIVERS_CFLAGS="$DRIVERS_CFLAGS -DDRIVER_PN533_USB_ENABLED"
                  ;;
    arygon)
                  DRIVERS_CFLAGS="$DRIVERS_CFLAGS -DDRIVER_ARYGON_ENABLED"
                  ;;
    pn532_uart)
                  DRIVERS_CFLAGS="$DRIVERS_CFLAGS -DDRIVER_PN532_UART_ENABLED"
                  ;;
    *)
                  AC_MSG_ERROR([Unknow driver: $driver])
                  ;;
    esac
  done
  AC_SUBST(DRIVERS_CFLAGS)
])
