/* -*- mode: c; c-file-style: "openbsd" -*- */
/*
 * Copyright (c) 2015 Nicira, Inc.
 * Copyright (c) 2008 Vincent Bernat <bernat@luffy.cx>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <config.h>
#include "lldpd-structs.h"
#include <stdlib.h>
#include <unistd.h>
#include "lldpd.h"
#include "timeval.h"

VLOG_DEFINE_THIS_MODULE(lldpd_structs);

void
lldpd_chassis_mgmt_cleanup(struct lldpd_chassis *chassis)
{
    struct lldpd_mgmt *mgmt, *mgmt_next;

    VLOG_DBG("cleanup management addresses for chassis %s",
             chassis->c_name ? chassis->c_name : "(unknown)");

    LIST_FOR_EACH_SAFE (mgmt, mgmt_next, m_entries, &chassis->c_mgmt) {
       list_remove(&mgmt->m_entries);
       free(mgmt);
    }

    list_init(&chassis->c_mgmt);
}

void
lldpd_chassis_cleanup(struct lldpd_chassis *chassis, bool all)
{
    lldpd_chassis_mgmt_cleanup(chassis);
    VLOG_DBG("cleanup chassis %s",
             chassis->c_name ? chassis->c_name : "(unknown)");
    free(chassis->c_id);
    free(chassis->c_name);
    free(chassis->c_descr);
    if (all) {
        free(chassis);
    }
}

/* Cleanup a remote port. The before last argument, `expire` is a function that
 * should be called when a remote port is removed. If the last argument is
 * true, all remote ports are removed.
 */
void
lldpd_remote_cleanup(struct lldpd_hardware *hw,
                     void(*expire)(struct lldpd_hardware *,
                                   struct lldpd_port *),
                     bool all)
{
    struct lldpd_port *port, *port_next;
    time_t now = time_now();

    VLOG_DBG("cleanup remote port on %s", hw->h_ifname);
    LIST_FOR_EACH_SAFE (port, port_next, p_entries, &hw->h_rports) {
        bool del = all;
        if (!all && expire &&
            (now >= port->p_lastupdate + port->p_chassis->c_ttl)) {
            hw->h_ageout_cnt++;
            hw->h_delete_cnt++;
            del = true;
        }
        if (del) {
            if (expire) {
                expire(hw, port);
            }

            if (!all) {
                list_remove(&port->p_entries);
            }
            lldpd_port_cleanup(port, true);
            free(port);
        }
    }
    if (all) {
        list_init(&hw->h_rports);
    }
}

/* If `all' is true, clear all information, including information that
   are not refreshed periodically. Port should be freed manually. */
void
lldpd_port_cleanup(struct lldpd_port *port, bool all)
{
    /* We set these to NULL so we don't free wrong memory */

    free(port->p_id);
    port->p_id = NULL;
    free(port->p_descr);
    port->p_descr = NULL;
    if (all) {
        free(port->p_lastframe);
        /* Chassis may not have been attributed, yet.*/
        if (port->p_chassis) {
            port->p_chassis->c_refcount--;
            port->p_chassis = NULL;
        }
    }
}

void
lldpd_config_cleanup(struct lldpd_config *config)
{
    VLOG_DBG("general configuration cleanup");
    free(config->c_mgmt_pattern);
    free(config->c_cid_pattern);
    free(config->c_iface_pattern);
    free(config->c_platform);
    free(config->c_description);
}