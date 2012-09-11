/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * mmcli -- Control sms status & access information from the command line
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2012 Google, Inc.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include <libmm-glib.h>

#include "mmcli.h"
#include "mmcli-common.h"

/* Context */
typedef struct {
    MMManager *manager;
    MMObject *object;
    GCancellable *cancellable;
    MMSms *sms;
} Context;
static Context *ctx;

/* Options */
static gboolean info_flag; /* set when no action found */
static gboolean send_flag;
static gboolean store_flag;
static gchar *store_in_storage_str;

static GOptionEntry entries[] = {
    { "send", 0, 0, G_OPTION_ARG_NONE, &send_flag,
      "Send SMS.",
      NULL,
    },
    { "store", 0, 0, G_OPTION_ARG_NONE, &store_flag,
      "Store the SMS in the device, at the default storage",
      NULL,
    },
    { "store-in-storage", 0, 0, G_OPTION_ARG_STRING, &store_in_storage_str,
      "Store the SMS in the device, at the specified storage",
      NULL,
    },
    { NULL }
};

GOptionGroup *
mmcli_sms_get_option_group (void)
{
	GOptionGroup *group;

	/* Status options */
	group = g_option_group_new ("sms",
	                            "SMS options",
	                            "Show SMS options",
	                            NULL,
	                            NULL);
	g_option_group_add_entries (group, entries);

	return group;
}

gboolean
mmcli_sms_options_enabled (void)
{
    static guint n_actions = 0;
    static gboolean checked = FALSE;

    if (checked)
        return !!n_actions;

    n_actions = (send_flag +
                 store_flag +
                 !!store_in_storage_str);

    if (n_actions == 0 && mmcli_get_common_sms_string ()) {
        /* default to info */
        info_flag = TRUE;
        n_actions++;
    }

    if (n_actions > 1) {
        g_printerr ("error: too many SMS actions requested\n");
        exit (EXIT_FAILURE);
    }

    if (info_flag)
        mmcli_force_sync_operation ();

    checked = TRUE;
    return !!n_actions;
}

static void
context_free (Context *ctx)
{
    if (!ctx)
        return;

    if (ctx->cancellable)
        g_object_unref (ctx->cancellable);
    if (ctx->sms)
        g_object_unref (ctx->sms);
    if (ctx->object)
        g_object_unref (ctx->object);
    if (ctx->manager)
        g_object_unref (ctx->manager);
    g_free (ctx);
}

void
mmcli_sms_shutdown (void)
{
    context_free (ctx);
}

static void
print_sms_info (MMSms *sms)
{
    MMSmsPduType pdu_type;

    /* Not the best thing to do, as we may be doing _get() calls twice, but
     * easiest to maintain */
#undef VALIDATE
#define VALIDATE(str) (str ? str : "unknown")

    pdu_type = mm_sms_get_pdu_type (sms);

    g_print ("SMS '%s'\n",
             mm_sms_get_path (sms));
    g_print ("  -----------------------------------\n"
             "  Content    |                text: '%s'\n"
             "             |              number: '%s'\n"
             "  -----------------------------------\n"
             "  Properties |            PDU type: '%s'\n"
             "             |               state: '%s'\n"
             "             |                smsc: '%s'\n"
             "             |            validity: '%u'\n"
             "             |               class: '%u'\n"
             "             |             storage: '%s'\n",
             VALIDATE (mm_sms_get_text (sms)),
             VALIDATE (mm_sms_get_number (sms)),
             mm_sms_pdu_type_get_string (pdu_type),
             mm_sms_state_get_string (mm_sms_get_state (sms)),
             VALIDATE (mm_sms_get_smsc (sms)),
             mm_sms_get_validity (sms),
             mm_sms_get_class (sms),
             mm_sms_storage_get_string (mm_sms_get_storage (sms)));

    if (pdu_type == MM_SMS_PDU_TYPE_SUBMIT)
        g_print ("             |     delivery report: '%s'\n",
                 mm_sms_get_delivery_report_request (sms) ? "requested" : "not requested");

    if (pdu_type == MM_SMS_PDU_TYPE_STATUS_REPORT ||
        pdu_type == MM_SMS_PDU_TYPE_SUBMIT)
        g_print ("             |   message reference: '%u'\n",
                 mm_sms_get_message_reference (sms));

    if (pdu_type == MM_SMS_PDU_TYPE_STATUS_REPORT ||
        pdu_type == MM_SMS_PDU_TYPE_DELIVER)
        g_print ("             |           timestamp: '%s'\n",
                 VALIDATE (mm_sms_get_timestamp (sms)));

    if (pdu_type == MM_SMS_PDU_TYPE_STATUS_REPORT) {
        g_print ("             |      delivery state: '%s' (0x%X)\n",
                 VALIDATE (mm_sms_delivery_state_get_string_extended (mm_sms_get_delivery_state (sms))),
                 mm_sms_get_delivery_state (sms));
        g_print ("             | discharge timestamp: '%s'\n",
                 VALIDATE (mm_sms_get_discharge_timestamp (sms)));
    }
}

static void
send_process_reply (gboolean      result,
                    const GError *error)
{
    if (!result) {
        g_printerr ("error: couldn't send the SMS: '%s'\n",
                    error ? error->message : "unknown error");
        exit (EXIT_FAILURE);
    }

    g_print ("successfully sent the SMS\n");
}

static void
send_ready (MMSms        *sms,
            GAsyncResult *result,
            gpointer      nothing)
{
    gboolean operation_result;
    GError *error = NULL;

    operation_result = mm_sms_send_finish (sms, result, &error);
    send_process_reply (operation_result, error);

    mmcli_async_operation_done ();
}

static void
store_process_reply (gboolean      result,
                     const GError *error)
{
    if (!result) {
        g_printerr ("error: couldn't store the SMS: '%s'\n",
                    error ? error->message : "unknown error");
        exit (EXIT_FAILURE);
    }

    g_print ("successfully stored the SMS\n");
}

static void
store_ready (MMSms        *sms,
             GAsyncResult *result,
             gpointer      nothing)
{
    gboolean operation_result;
    GError *error = NULL;

    operation_result = mm_sms_store_finish (sms, result, &error);
    store_process_reply (operation_result, error);

    mmcli_async_operation_done ();
}

static void
get_sms_ready (GObject      *source,
               GAsyncResult *result,
               gpointer      none)
{
    ctx->sms = mmcli_get_sms_finish (result,
                                     &ctx->manager,
                                     &ctx->object);
    /* Setup operation timeout */
    mmcli_force_operation_timeout (G_DBUS_PROXY (ctx->sms));

    if (info_flag)
        g_assert_not_reached ();

    /* Requesting to send the SMS? */
    if (send_flag) {
        mm_sms_send (ctx->sms,
                     ctx->cancellable,
                     (GAsyncReadyCallback)send_ready,
                     NULL);
        return;
    }

    /* Requesting to store the SMS? */
    if (store_flag) {
        mm_sms_store (ctx->sms,
                      MM_SMS_STORAGE_UNKNOWN,
                      ctx->cancellable,
                      (GAsyncReadyCallback)store_ready,
                      NULL);
        return;
    }

    /* Requesting to store the SMS in a specific storage? */
    if (store_in_storage_str) {
        MMSmsStorage storage;
        GError *error = NULL;

        storage = mm_common_get_sms_storage_from_string (store_in_storage_str, &error);
        if (error) {
            g_printerr ("error: couldn't store the SMS: '%s'\n",
                        error->message);
            exit (EXIT_FAILURE);
        }

        mm_sms_store (ctx->sms,
                      storage,
                      ctx->cancellable,
                      (GAsyncReadyCallback)store_ready,
                      NULL);
        return;
    }

    g_warn_if_reached ();
}

void
mmcli_sms_run_asynchronous (GDBusConnection *connection,
                            GCancellable    *cancellable)
{
    /* Initialize context */
    ctx = g_new0 (Context, 1);
    if (cancellable)
        ctx->cancellable = g_object_ref (cancellable);

    /* Get proper sms */
    mmcli_get_sms (connection,
                   mmcli_get_common_sms_string (),
                   cancellable,
                   (GAsyncReadyCallback)get_sms_ready,
                   NULL);
}

void
mmcli_sms_run_synchronous (GDBusConnection *connection)
{
    GError *error = NULL;

    /* Initialize context */
    ctx = g_new0 (Context, 1);
    ctx->sms = mmcli_get_sms_sync (connection,
                                   mmcli_get_common_sms_string (),
                                   &ctx->manager,
                                   &ctx->object);

    /* Setup operation timeout */
    mmcli_force_operation_timeout (G_DBUS_PROXY (ctx->sms));

    /* Request to get info from SMS? */
    if (info_flag) {
        g_debug ("Printing SMS info...");
        print_sms_info (ctx->sms);
        return;
    }

    /* Requesting to send the SMS? */
    if (send_flag) {
        gboolean operation_result;

        operation_result = mm_sms_send_sync (ctx->sms,
                                             NULL,
                                             &error);
        send_process_reply (operation_result, error);
        return;
    }

    /* Requesting to store the SMS? */
    if (store_flag) {
        gboolean operation_result;

        operation_result = mm_sms_store_sync (ctx->sms,
                                              MM_SMS_STORAGE_UNKNOWN,
                                              NULL,
                                              &error);
        store_process_reply (operation_result, error);
        return;
    }

    /* Requesting to store the SMS in a specific storage? */
    if (store_in_storage_str) {
        gboolean operation_result;
        MMSmsStorage storage;
        GError *error = NULL;

        storage = mm_common_get_sms_storage_from_string (store_in_storage_str, &error);
        if (error) {
            g_printerr ("error: couldn't store the SMS: '%s'\n",
                        error->message);
            exit (EXIT_FAILURE);
        }

        operation_result = mm_sms_store_sync (ctx->sms,
                                              storage,
                                              NULL,
                                              &error);
        store_process_reply (operation_result, error);
        return;
    }

    g_warn_if_reached ();
}
