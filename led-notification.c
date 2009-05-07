/*
 * Led-notification
 * Copyright (C) 2006  Simo Mattila
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Simo Mattila <simo.h.mattila@gmail.com>
 *
 * set ts=4 ;)
 */

#define PURPLE_PLUGINS

#define VERSION "0.1"

#include <glib.h>
#include <string.h>
#include <stdio.h>

#include "notify.h"
#include "plugin.h"
#include "version.h"
#include "debug.h"
#include "cmds.h"
#include "gtkconv.h"
#include "prefs.h"
#include "gtkprefs.h"
#include "gtkutils.h"
#include "gtkplugin.h"
#include "gtkblist.h"

void led_set(gboolean state) {
	const char *filename=purple_prefs_get_string(
	                                 "/plugins/gtk/gtk-simom-lednot/filename");
	FILE *file=NULL;

	file=fopen(filename, "w");
	if(file==NULL) {
		purple_debug_error("Led-notification",
		                 "Error opening file '%s'\n",filename);
		return;
	}

	if(state) {
		fputs("1", file);
	} else {
		fputs("0", file);
	}

	fclose(file);
}

GList *get_pending_list(guint max) {
	const char *im=purple_prefs_get_string("/plugins/gtk/gtk-simom-lednot/im");
	const char *chat=purple_prefs_get_string(
	                                     "/plugins/gtk/gtk-simom-lednot/chat");
	GList *l_im = NULL;
	GList *l_chat = NULL;

	
	if (im != NULL && strcmp(im, "always") == 0) {
		l_im = pidgin_conversations_find_unseen_list(PURPLE_CONV_TYPE_IM,
		                                             PIDGIN_UNSEEN_TEXT,
		                                             FALSE, max);
	} else if (im != NULL && strcmp(im, "hidden") == 0) {
		l_im = pidgin_conversations_find_unseen_list(PURPLE_CONV_TYPE_IM,
		                                             PIDGIN_UNSEEN_TEXT,
		                                             TRUE, max);
	}

	if (chat != NULL && strcmp(chat, "always") == 0) {
		l_chat = pidgin_conversations_find_unseen_list(PURPLE_CONV_TYPE_CHAT,
		                                               PIDGIN_UNSEEN_TEXT,
		                                               FALSE, max);
	} else if (chat != NULL && strcmp(chat, "nick") == 0) {
		l_chat = pidgin_conversations_find_unseen_list(PURPLE_CONV_TYPE_CHAT,
		                                               PIDGIN_UNSEEN_NICK,
		                                               FALSE, max);
	}

	if (l_im != NULL && l_chat != NULL)
		return g_list_concat(l_im, l_chat);
	else if (l_im != NULL)
		return l_im;
	else
		return l_chat;
}

static void lednot_conversation_updated(PurpleConversation *conv, 
                                        PurpleConvUpdateType type) {
	GList *list;

	if( type != PURPLE_CONV_UPDATE_UNSEEN ) {
		return;
	}

#if 0
	purple_debug_info("Led-notification", "Change in unseen conversations\n");
#endif

	list=get_pending_list(1);

	if(list==NULL) {
		led_set(FALSE);
	} else if(list!=NULL) {
		led_set(TRUE);
	}
	g_list_free(list);
}

static GtkWidget *plugin_config_frame(PurplePlugin *plugin) {
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *vbox2;
	GtkSizeGroup *sg;
	GtkWidget *dd;
	GtkWidget *ent;

	frame = gtk_vbox_new(FALSE, 18);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 12);

	vbox = pidgin_make_frame(frame, "Inform about unread...");
	vbox2 = pidgin_make_frame(frame, "Hardware setup:");
	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	dd = pidgin_prefs_dropdown(vbox, "Instant Messages:",
	                           PURPLE_PREF_STRING,
	                           "/plugins/gtk/gtk-simom-lednot/im",
	                           "Never", "never",
	                           "In hidden conversations", "hidden",
	                           "Always", "always",
	                           NULL);
	gtk_size_group_add_widget(sg, dd);

	dd = pidgin_prefs_dropdown(vbox, "Chat Messages:",
	                        PURPLE_PREF_STRING,
	                        "/plugins/gtk/gtk-simom-lednot/chat",
	                        "Never", "never",
	                        "When my nick is said", "nick",
	                        "Always", "always",
	                        NULL);
	gtk_size_group_add_widget(sg, dd);

	ent=pidgin_prefs_labeled_entry(vbox2,"File to control led:",
	                              "/plugins/gtk/gtk-simom-lednot/filename",sg);

	gtk_widget_show_all(frame);
	return frame;
}

static void init_plugin(PurplePlugin *plugin) {
	purple_prefs_add_none("/plugins/gtk/gtk-simom-lednot");
	purple_prefs_add_string("/plugins/gtk/gtk-simom-lednot/im", "always");
	purple_prefs_add_string("/plugins/gtk/gtk-simom-lednot/chat", "nick");
	purple_prefs_add_string("/plugins/gtk/gtk-simom-lednot/filename",
	                        "/proc/acpi/asus/mled");
}

static gboolean plugin_load(PurplePlugin *plugin) {
	purple_signal_connect(purple_conversations_get_handle(),
	                      "conversation-updated", plugin,
	                      PURPLE_CALLBACK(lednot_conversation_updated), NULL);
	return TRUE;
}

static gboolean plugin_unload(PurplePlugin *plugin) {
	led_set(FALSE); /* Turn the led off */
	purple_signal_disconnect(purple_conversations_get_handle(),
	                         "conversation-updated", plugin,
	                         PURPLE_CALLBACK(lednot_conversation_updated));
    return TRUE;
}

static PidginPluginUiInfo ui_info = {
	plugin_config_frame,
	0 /* page_num (Reserved) */
};

static PurplePluginInfo info = {
    PURPLE_PLUGIN_MAGIC,
    PURPLE_MAJOR_VERSION,
    PURPLE_MINOR_VERSION,
    PURPLE_PLUGIN_STANDARD,
    PIDGIN_PLUGIN_TYPE,
    0,
    NULL,
    PURPLE_PRIORITY_DEFAULT,

    "gtk-simom-lednot",
    "Led-notification",
    VERSION,

    "Led notification on laptops",
    "Informs for new messages with your laptops led",
    "Simo Mattila <simo.h.mattila@gmail.com>",
    "http://koti.mbnet.fi/simom/",

    plugin_load,   /* load */
    plugin_unload, /* unload */
    NULL,          /* destroy */

    &ui_info,
    NULL,
    NULL,
    NULL
};

PURPLE_INIT_PLUGIN(lednot, init_plugin, info);

