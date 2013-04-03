dnl Handle drivers arguments list

AC_DEFUN([LIBNFC_ARG_WITH_DRIVERS],
[
  AC_MSG_CHECKING(which drivers to build)
  AC_ARG_WITH(drivers,
  AS_HELP_STRING([--with-drivers=DRIVERS], [Use a custom driver set, where DRIVERS is a comma-separated list of drivers to build support for. Available drivers are: 'acr122_pcsc', 'acr122_usb', 'acr122s', 'arygon', 'pn532_spi', 'pn532_uart' and 'pn53x_usb'. Default drivers set is 'acr122_usb,acr122s,arygon,pn532_spi,pn532_uart,pn53x_usb'. The special driver set 'all' compile all available drivers.]),
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
                  DRIVER_BUILD_LIST="acr122_usb acr122s arygon pn53x_usb pn532_uart"
                  if test x"$spi_available" = x"yes"
                  then
                      DRIVER_BUILD_LIST="$DRIVER_BUILD_LIST pn532_spi"
                  fi
                  ;;
    all)
                  DRIVER_BUILD_LIST="acr122_pcsc acr122_usb acr122s arygon pn53x_usb pn532_uart"
                  if test x"$spi_available" = x"yes"
                  then
                      DRIVER_BUILD_LIST="$DRIVER_BUILD_LIST pn532_spi"
                  fi
                  ;;
  esac

  DRIVERS_CFLAGS=""

  driver_acr122_pcsc_enabled="no"
  driver_acr122_usb_enabled="no"
  driver_acr122s_enabled="no"
  driver_pn53x_usb_enabled="no"
  driver_arygon_enabled="no"
  driver_pn532_uart_enabled="no"
  driver_pn532_spi_enabled="no"

  for driver in ${DRIVER_BUILD_LIST}
  do
    case "${driver}" in
    acr122_pcsc)
                  pcsc_required="yes"
                  driver_acr122_pcsc_enabled="yes"
                  DRIVERS_CFLAGS="$DRIVERS_CFLAGS -DDRIVER_ACR122_PCSC_ENABLED"
                  ;;
    acr122_usb)
                  libusb_required="yes"
                  driver_acr122_usb_enabled="yes"
                  DRIVERS_CFLAGS="$DRIVERS_CFLAGS -DDRIVER_ACR122_USB_ENABLED"
                  ;;
    acr122s)
                  uart_required="yes"
                  driver_acr122s_enabled="yes"
                  DRIVERS_CFLAGS="$DRIVERS_CFLAGS -DDRIVER_ACR122S_ENABLED"
                  ;;
    pn53x_usb)
                  libusb_required="yes"
                  driver_pn53x_usb_enabled="yes"
                  DRIVERS_CFLAGS="$DRIVERS_CFLAGS -DDRIVER_PN53X_USB_ENABLED"
                  ;;
    arygon)
                  uart_required="yes"
                  driver_arygon_enabled="yes"
                  DRIVERS_CFLAGS="$DRIVERS_CFLAGS -DDRIVER_ARYGON_ENABLED"
                  ;;
    pn532_uart)
                  uart_required="yes"
                  driver_pn532_uart_enabled="yes"
                  DRIVERS_CFLAGS="$DRIVERS_CFLAGS -DDRIVER_PN532_UART_ENABLED"
                  ;;
    pn532_spi)
                  spi_required="yes"
                  driver_pn532_spi_enabled="yes"
                  DRIVERS_CFLAGS="$DRIVERS_CFLAGS -DDRIVER_PN532_SPI_ENABLED"
                  ;;
    *)
                  AC_MSG_ERROR([Unknow driver: $driver])
                  ;;
    esac
  done
  AC_SUBST(DRIVERS_CFLAGS)
  AM_CONDITIONAL(DRIVER_ACR122_PCSC_ENABLED, [test x"$driver_acr122_pcsc_enabled" = xyes])
  AM_CONDITIONAL(DRIVER_ACR122_USB_ENABLED, [test x"$driver_acr122_usb_enabled" = xyes])
  AM_CONDITIONAL(DRIVER_ACR122S_ENABLED, [test x"$driver_acr122s_enabled" = xyes])
  AM_CONDITIONAL(DRIVER_PN53X_USB_ENABLED, [test x"$driver_pn53x_usb_enabled" = xyes])
  AM_CONDITIONAL(DRIVER_ARYGON_ENABLED, [test x"$driver_arygon_enabled" = xyes])
  AM_CONDITIONAL(DRIVER_PN532_UART_ENABLED, [test x"$driver_pn532_uart_enabled" = xyes])
  AM_CONDITIONAL(DRIVER_PN532_SPI_ENABLED, [test x"$driver_pn532_spi_enabled" = xyes])
])

AC_DEFUN([LIBNFC_DRIVERS_SUMMARY],[
echo
echo "Selected drivers:"
echo "   acr122_pcsc...... $driver_acr122_pcsc_enabled"
echo "   acr122_usb....... $driver_acr122_usb_enabled"
echo "   acr122s.......... $driver_acr122s_enabled"
echo "   arygon........... $driver_arygon_enabled"
echo "   pn53x_usb........ $driver_pn53x_usb_enabled"
echo "   pn532_uart....... $driver_pn532_uart_enabled"
echo "   pn532_spi.......  $driver_pn532_spi_enabled"
])
