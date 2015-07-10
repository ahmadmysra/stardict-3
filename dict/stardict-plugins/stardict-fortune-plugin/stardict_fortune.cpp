/*
 * Copyright 2015 huzheng <huzheng001@gmail.com>
 *
 * This file is part of StarDict.
 *
 * StarDict is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * StarDict is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with StarDict.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "stardict_fortune.h"
#include <glib/gi18n.h>
#include <stdlib.h>
#include <cstring>
#include <string>

static const StarDictPluginSystemInfo *plugin_info = NULL;
static IAppDirs* gpAppDirs = NULL;


static char *build_dictdata(char type, const char *definition)
{
	size_t len = strlen(definition);
	guint32 size;
	size = sizeof(char) + len + 1;
	char *data = (char *)g_malloc(sizeof(guint32) + size);
	char *p = data;
	*((guint32 *)p)= size;
	p += sizeof(guint32);
	*p = type;
	p++;
	memcpy(p, definition, len+1);
	return data;
}

static void terminal2pango(const char *t, std::string &pango)
{
	pango.clear();
	const char *p1;
	std::string color_num;
	std::string color;
	std::string tmp_str;
	while (*t) {
		if ((*t == '') && (t[1] == '[')) {
			p1 = strchr(t+2, 'm');
			if (p1) {
				color_num.assign(t+2, p1-t-2);
				if (color_num == "30") {
					color = "#000000";
				} else if (color_num == "31") {
					color = "#FF0000";
				} else if (color_num == "32") {
					color = "#008000";
				} else if (color_num == "33") {
					color = "#FFFF00";
				} else if (color_num == "34") {
					color = "#0000FF";
				} else if (color_num == "35") {
					color = "#FF00FF";
				} else if (color_num == "36") {
					color = "#00FFFF";
				} else if (color_num == "37") {
					color = "#FFFFFF";
				}
				t = p1 +1 ;
				pango += "<span foreground=\"";
				pango += color;
				pango += "\">";
				p1 = strstr(t, "[m");
				if (p1) {
					tmp_str.assign(t, p1-t);
					pango += tmp_str;
					t = p1 +3;
				}
				pango += "</span>";
			}
		} else {
			pango += *t;
			t++;
		}
	}
}

static void lookup(const char *text, char ***pppWord, char ****ppppWordData)
{
	std::string definition;
	bool found = true;
	FILE *pf = popen("fortune", "r");
	if (!pf) {
		found = false;
	} else {
		char buffer[2048];
		size_t len;
		while (true) {
			len = fread(buffer, 1, sizeof(buffer), pf);
			if (len <= 0)
				break;
			definition.append(buffer, len);
		}
		pclose(pf);
		if (definition.empty()) {
			found = false;
		} else {
			size_t length = definition.length();
			if (definition[length-1] == '\n') {
				definition.resize(length-1, '\0');
			}
		}
	}
	std::string pango;
	if (found) {
		terminal2pango(definition.c_str(), pango);
	} else {
		pango = _("<b><span foreground=\"red\">fortune</span> program is not found!</b>");
	}
	*pppWord = (gchar **)g_malloc(sizeof(gchar *)*2);
	(*pppWord)[0] = g_strdup(text);
	(*pppWord)[1] = NULL;
	*ppppWordData = (gchar ***)g_malloc(sizeof(gchar **)*(1));
	(*ppppWordData)[0] = (gchar **)g_malloc(sizeof(gchar *)*2);
	(*ppppWordData)[0][0] =  build_dictdata('g', pango.c_str());
	(*ppppWordData)[0][1] = NULL;
}

static void configure()
{
	GtkWidget *window = gtk_dialog_new_with_buttons(_("Fortune configuration"), GTK_WINDOW(plugin_info->pluginwin), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
#else
	GtkWidget *vbox = gtk_vbox_new(false, 5);
#endif
	GtkWidget *label;
	label = gtk_label_new(NULL);
	int have_fortune;
	have_fortune = system("which fortune");
	if (have_fortune == 0) {
		gtk_label_set_markup(GTK_LABEL (label), _("<b><span foreground=\"yellow\">fortune</span> program is found!</b>"));
	} else {
		gtk_label_set_markup(GTK_LABEL (label), _("<b><span foreground=\"red\">fortune</span> program is not found!</b>"));
	}
	gtk_box_pack_start(GTK_BOX(vbox), label, false, false, 0);
	gtk_widget_show_all(vbox);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area(GTK_DIALOG(window))), vbox);
	gtk_dialog_run(GTK_DIALOG(window));
	gtk_widget_destroy (window);
}

bool stardict_plugin_init(StarDictPlugInObject *obj, IAppDirs* appDirs)
{
	g_debug(_("Loading Fortune plug-in..."));
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print(_("Error: Fortune plugin version doesn't match!\n"));
		return true;
	}
	obj->type = StarDictPlugInType_VIRTUALDICT;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng001@gmail.com&gt;</author><website>http://www.stardict.org</website></plugin_info>", _("Fortune"), _("Fortune virtual dictionary."), _("Show the fortune."));
	obj->configure_func = configure;
	plugin_info = obj->plugin_info;
	gpAppDirs = appDirs;

	return false;
}

void stardict_plugin_exit(void)
{
	gpAppDirs = NULL;
}

bool stardict_virtualdict_plugin_init(StarDictVirtualDictPlugInObject *obj)
{
	obj->lookup_func = lookup;
	obj->dict_name = _("Fortune");
	g_print(_("Fortune plug-in loaded.\n"));
	return false;
}
