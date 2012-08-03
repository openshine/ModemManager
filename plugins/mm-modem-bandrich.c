/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details:
 *
 * Copyright (C) 2008 - 2009 Novell, Inc.
 * Copyright (C) 2009 - 2012 Red Hat, Inc.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <dbus/dbus-glib.h>

#include "mm-modem-bandrich.h"
#include "mm-modem-simple.h"
#include "mm-errors.h"
#include "mm-callback-info.h"
#include "mm-modem-gsm-card.h"
#include "mm-log.h"
#include "mm-modem-icera.h"

static void modem_init (MMModem *modem_class);
static void modem_gsm_network_init (MMModemGsmNetwork *gsm_network_class);
static void modem_simple_init (MMModemSimple *class);
static void modem_gsm_card_init (MMModemGsmCard *class);
static void modem_icera_init (MMModemIcera *icera_class);

G_DEFINE_TYPE_EXTENDED (MMModemBandrich, mm_modem_bandrich, MM_TYPE_GENERIC_GSM, 0,
                        G_IMPLEMENT_INTERFACE (MM_TYPE_MODEM, modem_init)
                        G_IMPLEMENT_INTERFACE (MM_TYPE_MODEM_SIMPLE, modem_simple_init)
                        G_IMPLEMENT_INTERFACE (MM_TYPE_MODEM_ICERA, modem_icera_init)
                        G_IMPLEMENT_INTERFACE (MM_TYPE_MODEM_GSM_NETWORK, modem_gsm_network_init)
                        G_IMPLEMENT_INTERFACE (MM_TYPE_MODEM_GSM_CARD, modem_gsm_card_init))

#define MM_MODEM_BANDRICH_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MM_TYPE_MODEM_BANDRICH, MMModemBandrichPrivate))

typedef struct {
    gboolean disposed;

    MMModemIceraPrivate *icera;
    gboolean has_net;
    gboolean is_icera;
} MMModemBandrichPrivate;


MMModem *
mm_modem_bandrich_new (const char *device,
                       const char *driver,
                       const char *plugin)
{
    MMModem *modem;

    g_return_val_if_fail (device != NULL, NULL);
    g_return_val_if_fail (driver != NULL, NULL);
    g_return_val_if_fail (plugin != NULL, NULL);

    modem = MM_MODEM (g_object_new (MM_TYPE_MODEM_BANDRICH,
                                    MM_MODEM_MASTER_DEVICE, device,
                                    MM_MODEM_DRIVER, driver,
                                    MM_MODEM_PLUGIN, plugin,
                                    MM_MODEM_IP_METHOD, MM_MODEM_IP_METHOD_DHCP,
                                    NULL));
    if (modem)
        MM_MODEM_BANDRICH_GET_PRIVATE (modem)->icera = mm_modem_icera_init_private ();

    return modem;
}

static void
get_allowed_mode (MMGenericGsm *gsm,
                  MMModemUIntFn callback,
                  gpointer user_data)
{
    MMCallbackInfo *info;

    if (MM_MODEM_BANDRICH_GET_PRIVATE (gsm)->is_icera)
        mm_modem_icera_get_allowed_mode (MM_MODEM_ICERA (gsm), callback, user_data);
    else {
        info = mm_callback_info_uint_new (MM_MODEM (gsm), callback, user_data);
        info->error = g_error_new_literal (MM_MODEM_ERROR, MM_MODEM_ERROR_OPERATION_NOT_SUPPORTED,
                                           "Operation not supported");
        mm_callback_info_schedule (info);
    }
}

static void
set_allowed_mode (MMGenericGsm *gsm,
                  MMModemGsmAllowedMode mode,
                  MMModemFn callback,
                  gpointer user_data)
{
    MMCallbackInfo *info;

    if (MM_MODEM_BANDRICH_GET_PRIVATE (gsm)->is_icera)
        mm_modem_icera_set_allowed_mode (MM_MODEM_ICERA (gsm), mode, callback, user_data);
    else {
        info = mm_callback_info_new (MM_MODEM (gsm), callback, user_data);
        info->error = g_error_new_literal (MM_MODEM_ERROR, MM_MODEM_ERROR_OPERATION_NOT_SUPPORTED,
                                           "Operation not supported");
        mm_callback_info_schedule (info);
    }
}

static void
get_unlock_retries (MMModemGsmCard *modem,
                    MMModemArrayFn callback,
                    gpointer user_data)
{
    MMCallbackInfo *info;

    if (MM_MODEM_BANDRICH_GET_PRIVATE (modem)->is_icera)
        mm_modem_icera_get_unlock_retries (MM_MODEM_ICERA (modem), callback, user_data);
    else {
        info = mm_callback_info_array_new (MM_MODEM (modem), callback, user_data);
        info->error = g_error_new_literal (MM_MODEM_ERROR, MM_MODEM_ERROR_OPERATION_NOT_SUPPORTED,
                                           "Operation not supported");
        mm_callback_info_schedule (info);
    }
}

static void
get_access_technology (MMGenericGsm *gsm,
                       MMModemUIntFn callback,
                       gpointer user_data)
{
    MMCallbackInfo *info;

    if (MM_MODEM_BANDRICH_GET_PRIVATE (gsm)->is_icera)
        mm_modem_icera_get_access_technology (MM_MODEM_ICERA (gsm), callback, user_data);
    else {
        info = mm_callback_info_uint_new (MM_MODEM (gsm), callback, user_data);
        info->error = g_error_new_literal (MM_MODEM_ERROR, MM_MODEM_ERROR_OPERATION_NOT_SUPPORTED,
                                           "Operation not supported");
        mm_callback_info_schedule (info);
    }
}

static void
set_band (MMModemGsmNetwork *modem,
          MMModemGsmBand band,
          MMModemFn callback,
          gpointer user_data)
{
    MMCallbackInfo *info;

    if (MM_MODEM_BANDRICH_GET_PRIVATE (modem)->is_icera)
        mm_modem_icera_set_band (MM_MODEM_ICERA (modem), band, callback, user_data);
    else {
        info = mm_callback_info_new (MM_MODEM (modem), callback, user_data);
        info->error = g_error_new_literal (MM_MODEM_ERROR, MM_MODEM_ERROR_OPERATION_NOT_SUPPORTED,
                                           "Operation not supported");
        mm_callback_info_schedule (info);
    }
}

static void
get_band (MMModemGsmNetwork *modem,
          MMModemUIntFn callback,
          gpointer user_data)
{
    MMCallbackInfo *info;

    if (MM_MODEM_BANDRICH_GET_PRIVATE (modem)->is_icera)
        mm_modem_icera_get_current_bands (MM_MODEM_ICERA (modem), callback, user_data);
    else {
        info = mm_callback_info_uint_new (MM_MODEM (modem), callback, user_data);
        info->error = g_error_new_literal (MM_MODEM_ERROR, MM_MODEM_ERROR_OPERATION_NOT_SUPPORTED,
                                           "Operation not supported");
        mm_callback_info_schedule (info);
    }
}

static void
get_supported_bands (MMGenericGsm *gsm,
                     MMModemUIntFn callback,
                     gpointer user_data)
{
    MMCallbackInfo *info;

    if (MM_MODEM_BANDRICH_GET_PRIVATE (gsm)->is_icera)
        mm_modem_icera_get_supported_bands (MM_MODEM_ICERA (gsm), callback, user_data);
    else {
        info = mm_callback_info_uint_new (MM_MODEM (gsm), callback, user_data);
        info->error = g_error_new_literal (MM_MODEM_ERROR, MM_MODEM_ERROR_OPERATION_NOT_SUPPORTED,
                                           "Operation not supported");
        mm_callback_info_schedule (info);
    }
}

static void
do_disconnect (MMGenericGsm *gsm,
               gint cid,
               MMModemFn callback,
               gpointer user_data)
{
    MMModemBandrichPrivate *priv = MM_MODEM_BANDRICH_GET_PRIVATE (gsm);

    if (priv->is_icera && priv->has_net)
        mm_modem_icera_do_disconnect (gsm, cid, callback, user_data);
    else
        MM_GENERIC_GSM_CLASS (mm_modem_bandrich_parent_class)->do_disconnect (gsm, cid, callback, user_data);
}

/*****************************************************************************/

static void
init_all_done (MMAtSerialPort *port,
               GString *response,
               GError *error,
               gpointer user_data)
{
    MMCallbackInfo *info = (MMCallbackInfo *) user_data;

    /* If the modem has already been removed, return without
     * scheduling callback */
    if (mm_callback_info_check_modem_removed (info))
        return;

    if (!error && MM_MODEM_BANDRICH_GET_PRIVATE (info->modem)->is_icera)
        mm_modem_icera_change_unsolicited_messages (MM_MODEM_ICERA (info->modem), TRUE);

    mm_generic_gsm_enable_complete (MM_GENERIC_GSM (info->modem), error, info);
}

static void
icera_check_cb (MMModem *modem,
                guint32 result,
                GError *error,
                gpointer user_data)
{
    if (!error) {
        MMModemBandrich *self = MM_MODEM_BANDRICH (user_data);
        MMModemBandrichPrivate *priv = MM_MODEM_BANDRICH_GET_PRIVATE (self);

        if (result) {
            priv->is_icera = TRUE;

            /* Some devices with Icera chipsets don't have pseudo-ethernet
             * ports and thus should use PPP.  Some devices use static IP,
             * while others use DHCP on the net port.  What fun.
             */
            if (priv->has_net) {
                g_object_set (G_OBJECT (modem),
                              MM_MODEM_IP_METHOD, MM_MODEM_IP_METHOD_STATIC,
                              NULL);
            }
        }
    }
}

static void
init2_done (MMAtSerialPort *port,
            GString *response,
            GError *error,
            gpointer user_data)
{
    MMCallbackInfo *info = (MMCallbackInfo *) user_data;
    MMModemBandrich *self;

    /* If the modem has already been removed, return without
     * scheduling callback */
    if (mm_callback_info_check_modem_removed (info))
        return;

    self = MM_MODEM_BANDRICH (info->modem);

    if (error)
        mm_generic_gsm_enable_complete (MM_GENERIC_GSM (self), error, info);
    else {
        /* Finish the initialization */
        mm_modem_icera_is_icera (MM_MODEM_ICERA (self), icera_check_cb, self);
        mm_at_serial_port_queue_command (port, "E0 V1 X4 &C1", 3, init_all_done, info);
    }
}

static void
init_done (MMAtSerialPort *port,
           GString *response,
           GError *error,
           gpointer user_data)
{
    MMCallbackInfo *info = (MMCallbackInfo *) user_data;
    MMModemBandrich *self;

    /* If the modem has already been removed, return without
     * scheduling callback */
    if (mm_callback_info_check_modem_removed (info))
        return;

    self = MM_MODEM_BANDRICH (info->modem);

    if (error)
        mm_generic_gsm_enable_complete (MM_GENERIC_GSM (self), error, info);
    else {
        /* Power up the modem */
        mm_at_serial_port_queue_command (port, "+CMEE=1", 2, NULL, NULL);
        mm_at_serial_port_queue_command (port, "+CFUN=1", 10, init2_done, info);
    }
}

static void
do_enable (MMGenericGsm *modem, MMModemFn callback, gpointer user_data)
{
    MMCallbackInfo *info;
    MMAtSerialPort *primary;

    info = mm_callback_info_new (MM_MODEM (modem), callback, user_data);

    primary = mm_generic_gsm_get_at_port (modem, MM_AT_PORT_FLAG_PRIMARY);
    g_assert (primary);
    mm_at_serial_port_queue_command (primary, "Z", 3, NULL, NULL);
    mm_at_serial_port_queue_command (primary, "E0 V1", 3, init_done, info);
}

/*****************************************************************************/

static void
disable_unsolicited_done (MMAtSerialPort *port,
                          GString *response,
                          GError *error,
                          gpointer user_data)

{
    MMCallbackInfo *info = (MMCallbackInfo *) user_data;

    /* If the modem has already been removed, return without
     * scheduling callback */
    if (mm_callback_info_check_modem_removed (info))
        return;

    /* Ignore all errors */
    mm_callback_info_schedule (info);
}

static void
invoke_call_parent_disable_fn (MMCallbackInfo *info)
{
    /* Note: we won't call the parent disable if info->modem is no longer
     * valid. The invoke is called always once the info gets scheduled, which
     * may happen during removed modem detection. */
    if (info->modem) {
        MMModem *parent_modem_iface;

        parent_modem_iface = g_type_interface_peek_parent (MM_MODEM_GET_INTERFACE (info->modem));
        parent_modem_iface->disable (info->modem, (MMModemFn) info->callback, info->user_data);
    }
}


static void
disable (MMModem *modem,
         MMModemFn callback,
         gpointer user_data)
{
    MMModemBandrichPrivate *priv = MM_MODEM_BANDRICH_GET_PRIVATE (modem);
    MMAtSerialPort *primary;
    MMCallbackInfo *info;

    info = mm_callback_info_new_full (modem,
                                      invoke_call_parent_disable_fn,
                                      (GCallback)callback,
                                      user_data);

    primary = mm_generic_gsm_get_at_port (MM_GENERIC_GSM (modem), MM_AT_PORT_FLAG_PRIMARY);
    g_assert (primary);

    /* Turn off unsolicited responses */
    if (priv->is_icera) {
        mm_modem_icera_cleanup (MM_MODEM_ICERA (modem));
        mm_modem_icera_change_unsolicited_messages (MM_MODEM_ICERA (modem), FALSE);
    }

    /* Random command to ensure unsolicited message disable completes */
    mm_at_serial_port_queue_command (primary, "E0", 5, disable_unsolicited_done, info);
}

static void
do_connect (MMModem *modem,
            const char *number,
            MMModemFn callback,
            gpointer user_data)
{
    MMModem *parent_iface;
    MMModemBandrichPrivate *priv = MM_MODEM_BANDRICH_GET_PRIVATE (modem);

    if (priv->is_icera && priv->has_net)
        mm_modem_icera_do_connect (MM_MODEM_ICERA (modem), number, callback, user_data);
    else {
        parent_iface = g_type_interface_peek_parent (MM_MODEM_GET_INTERFACE (modem));
        parent_iface->connect (MM_MODEM (modem), number, callback, user_data);
    }
}

static void
get_ip4_config (MMModem *modem,
                MMModemIp4Fn callback,
                gpointer user_data)
{
    MMModem *parent_iface;

    if (MM_MODEM_BANDRICH_GET_PRIVATE (modem)->is_icera) {
        mm_modem_icera_get_ip4_config (MM_MODEM_ICERA (modem), callback, user_data);
    } else {
        parent_iface = g_type_interface_peek_parent (MM_MODEM_GET_INTERFACE (modem));
        parent_iface->get_ip4_config (MM_MODEM (modem), callback, user_data);
    }
}

/*****************************************************************************/

static void
simple_connect (MMModemSimple *simple,
                GHashTable *properties,
                MMModemFn callback,
                gpointer user_data)
{
    MMCallbackInfo *info = (MMCallbackInfo *) user_data;
    MMModemSimple *parent_iface;

    if (MM_MODEM_BANDRICH_GET_PRIVATE (simple)->is_icera)
        mm_modem_icera_simple_connect (MM_MODEM_ICERA (simple), properties);

    parent_iface = g_type_interface_peek_parent (MM_MODEM_SIMPLE_GET_INTERFACE (simple));
    parent_iface->connect (MM_MODEM_SIMPLE (simple), properties, callback, info);
}


/*****************************************************************************/

static void
port_grabbed (MMGenericGsm *gsm,
              MMPort *port,
              MMAtPortFlags pflags,
              gpointer user_data)
{
    if (MM_IS_AT_SERIAL_PORT (port)) {
        g_object_set (port, MM_PORT_CARRIER_DETECT, FALSE, NULL);

        /* Add Icera-specific handlers */
        mm_modem_icera_register_unsolicted_handlers (MM_MODEM_ICERA (gsm), MM_AT_SERIAL_PORT (port));
    } else if (mm_port_get_subsys (port) == MM_PORT_SUBSYS_NET)
        MM_MODEM_BANDRICH_GET_PRIVATE (gsm)->has_net = TRUE;
}

static void
set_property (GObject *object,
              guint prop_id,
              const GValue *value,
              GParamSpec *pspec)
{
    /* Do nothing... see set_property() in parent, which also does nothing */
}

static void
get_property (GObject *object,
              guint prop_id,
              GValue *value,
              GParamSpec *pspec)
{
    switch (prop_id) {
    case MM_GENERIC_GSM_PROP_POWER_DOWN_CMD:
        /* Use AT+CFUN=4 for power down. It will stop the RF (IMSI detach), and
         * keeps access to the SIM */
        g_value_set_string (value, "+CFUN=4");
        break;
    default:
        break;
    }
}

/*****************************************************************************/

static MMModemIceraPrivate *
get_icera_private (MMModemIcera *icera)
{
    return MM_MODEM_BANDRICH_GET_PRIVATE (icera)->icera;
}

/*****************************************************************************/

static void
modem_init (MMModem *modem_class)
{
    modem_class->disable = disable;
    modem_class->connect = do_connect;
    modem_class->get_ip4_config = get_ip4_config;
}

static void
modem_icera_init (MMModemIcera *icera)
{
    icera->get_private = get_icera_private;
}

static void
modem_simple_init (MMModemSimple *class)
{
    class->connect = simple_connect;
}

static void
modem_gsm_network_init (MMModemGsmNetwork *class)
{
    class->set_band = set_band;
    class->get_band = get_band;
}

static void
modem_gsm_card_init (MMModemGsmCard *class)
{
    class->get_unlock_retries = get_unlock_retries;
}

static void
mm_modem_bandrich_init (MMModemBandrich *self)
{
}

static void
dispose (GObject *object)
{
    MMModemBandrich *self = MM_MODEM_BANDRICH (object);
    MMModemBandrichPrivate *priv = MM_MODEM_BANDRICH_GET_PRIVATE (self);

    if (priv->disposed == FALSE) {
        priv->disposed = TRUE;

        mm_modem_icera_dispose_private (MM_MODEM_ICERA (self));
    }

    G_OBJECT_CLASS (mm_modem_bandrich_parent_class)->dispose (object);
}

static void
mm_modem_bandrich_class_init (MMModemBandrichClass *klass)
{

    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    MMGenericGsmClass *gsm_class = MM_GENERIC_GSM_CLASS (klass);

    mm_modem_bandrich_parent_class = g_type_class_peek_parent (klass);
    g_type_class_add_private (object_class, sizeof (MMModemBandrichPrivate));

    object_class->dispose = dispose;
    object_class->get_property = get_property;
    object_class->set_property = set_property;
    g_object_class_override_property (object_class,
                                      MM_GENERIC_GSM_PROP_POWER_DOWN_CMD,
                                      MM_GENERIC_GSM_POWER_DOWN_CMD);

    gsm_class->port_grabbed = port_grabbed;
    gsm_class->do_disconnect = do_disconnect;
    gsm_class->do_enable = do_enable;
    gsm_class->set_allowed_mode = set_allowed_mode;
    gsm_class->get_allowed_mode = get_allowed_mode;
    gsm_class->get_access_technology = get_access_technology;
    gsm_class->get_supported_bands = get_supported_bands;
}
