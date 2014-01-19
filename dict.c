#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

#define KEY_WORD_XPATH "//div[@id='phrsListTab']"
#define KEY_URL_PREFIX "http://dict.youdao.com/search?q="
#define KEY_URL_SUFIX "&keyfrom=dict.index"

GPtrArray * dict_get_translate(const gchar * keyword)
{
	gchar * key_url = NULL;
	htmlDocPtr html_doc = NULL;
	xmlDocPtr doc = NULL;
	xmlNodePtr clone = NULL;
	xmlXPathContextPtr ctx = NULL;
	xmlXPathObjectPtr obj = NULL;
	xmlNodeSetPtr nodeset = NULL;
	gchar * tmp_str = NULL;
	GPtrArray * result = NULL;
	gint i;


	key_url = g_strjoin(NULL, KEY_URL_PREFIX, keyword, KEY_URL_SUFIX, NULL);
	g_debug("KEY-URL: %s", key_url);

	html_doc = htmlReadFile(key_url, NULL, HTML_PARSE_NOWARNING | HTML_PARSE_NOERROR);
	if (!html_doc)
	{
		g_message("KEY-URL %s get failed", key_url);
		goto out;
	}

	ctx = xmlXPathNewContext((xmlDocPtr)html_doc);
	if (!ctx)
	{
		g_message("XPath context creat failed");
		goto out;
	}

	obj = xmlXPathEvalExpression(KEY_WORD_XPATH, ctx);
	if (!obj)
	{
		g_message("XPath eval key word xpath failed");
		goto out;
	}

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval))
	{
		g_message("XPath search keyword failed");
		goto out;
	}

	nodeset = obj->nodesetval;
	g_debug("Key word node set have %d object", nodeset->nodeNr);

	clone = xmlCopyNode(nodeset->nodeTab[0], 1);
	if (!clone)
		goto out;

	doc = xmlNewDoc("1.0");
	if (!doc)
		goto out;

	xmlDocSetRootElement(doc, clone);

	xmlXPathFreeContext(ctx);
	ctx = NULL;

	xmlXPathFreeObject(obj);
	obj = NULL;

	ctx = xmlXPathNewContext(doc);
	if (!ctx)
		goto out;

	obj = xmlXPathEvalExpression("//span[@class='keyword']", ctx);
	if (!obj)
		goto out;

	nodeset = obj->nodesetval;
	tmp_str = xmlNodeGetContent(nodeset->nodeTab[0]->xmlChildrenNode);
	g_debug("The word to search %s", tmp_str);

	xmlFree(tmp_str);
	tmp_str = NULL;

	xmlXPathFreeObject(obj);
	obj = NULL;

	obj = xmlXPathEvalExpression("//ul/li", ctx);
	if (!obj)
		goto out;

	if (xmlXPathNodeSetIsEmpty(obj->nodesetval))
	{
		g_message("Result value is empty");
		goto out;
	}

	nodeset = obj->nodesetval;
	result = g_ptr_array_sized_new(nodeset->nodeNr + 1);
	for (i = 0; i < nodeset->nodeNr; ++i)
	{
		tmp_str = xmlNodeGetContent(nodeset->nodeTab[i]->xmlChildrenNode);
		g_ptr_array_add(result, tmp_str);
	}

	g_ptr_array_add(result, NULL);

out:
	if (doc)
		xmlFreeDoc(doc);

	if (key_url)
		g_free(key_url);

	if (html_doc)
		xmlFreeDoc((xmlDocPtr)html_doc);

	if (ctx)
		xmlXPathFreeContext(ctx);

	if (obj)
		xmlXPathFreeObject(obj);

	return result;
}

void dict_present_result(gpointer data, gpointer user_data)
{
	g_printf("result: %s\n", data);
}

void dict_free_the_result(gpointer data, gpointer user_data)
{
	if (data)
		xmlFree(data);
}

#define DICTIONARY_TYPE_PANEL (dictionary_panel_get_type())
#define DICTIONARY_PANEL(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), DICTIONARY_TYPE_PANEL, DictionaryPanel))
#define DICTIONARY_IS_PANEL(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), DICTIONARY_TYPE_PANEL))
#define DICTIONARY_PANEL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), DICTIONARY_TYPE_PANEL, DictionaryPanelClass))
#define DICTIONARY_IS_PANEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), DICTIONARY_TYPE_PANEL))
#define DICTIONARY_PANEL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), DICTIONARY_TYPE_PANEL, DictionaryPanelClass))

typedef struct _DictionaryPanel DictionaryPanel;
typedef struct _DictionaryPanelClass DictionaryPanelClass;

struct _DictionaryPanel
{
	GtkWindow window;

	GtkWidget * entry;
	GtkWidget * text_view;
	GtkWidget * search_button;
};

struct _DictionaryPanelClass
{
	GtkWindowClass parent_class;
};

GType dictionary_panel_get_type(void);

static void dictionary_panel_class_init(DictionaryPanelClass * klass)
{

}

void dict_translate_word(GtkWidget * button, DictionaryPanel * panel)
{
	GPtrArray * result;
	GtkTextBuffer * text_buffer;
	const gchar * keyword = gtk_entry_get_text(GTK_ENTRY(panel->entry));
	gchar * tip;

	//here we should check the keyword
	if (strlen(keyword) == 0)
		return;

	text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(panel->text_view));

	result = dict_get_translate(keyword);
	if (result)
	{
		tip = g_strjoinv("\n", (gchar **)result->pdata);

		g_ptr_array_foreach(result, dict_free_the_result, NULL);

		g_ptr_array_free(result, TRUE);
	}
	else
	{
		tip = g_strjoin(NULL, "Word ", keyword, " not found", NULL);
	}

	gtk_text_buffer_set_text(text_buffer, tip, -1);
	g_free(tip);
}

gboolean dict_filter_input_word(GtkWidget * entry, GdkEventKey * key, DictionaryPanel * panel)
{
	switch (key->keyval)
	{
		case GDK_KEY_Return:
			dict_translate_word(NULL, panel);
			return TRUE;
		case GDK_KEY_L:
		case GDK_KEY_l:
			if (key->state & GDK_CONTROL_MASK)
			{
				gtk_entry_set_text(GTK_ENTRY(panel->entry), "");
				return TRUE;
			}

			break;
		default:
			break;
	}

	return FALSE;
}

static void dictionary_panel_init(DictionaryPanel * self)
{
	GtkWidget * hbox, * vbox;
	GtkWidget * scrolled_window;
	GtkWidget * window = GTK_WIDGET(&self->window);

	gtk_widget_set_size_request(window, 500, 300);
	gtk_container_set_border_width(GTK_CONTAINER(window), 10);
	gtk_window_set_title(GTK_WINDOW(window), "Dictionary");
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);


	self->entry = gtk_entry_new();
	gtk_widget_add_events(self->entry, GDK_KEY_PRESS_MASK);
	g_signal_connect(window, "key-press-event", G_CALLBACK(dict_filter_input_word), self);

	self->text_view = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(self->text_view), FALSE);

	self->search_button = gtk_button_new_from_stock(GTK_STOCK_FIND);
	g_signal_connect(self->search_button, "clicked", G_CALLBACK(dict_translate_word), self);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
	gtk_box_pack_start(GTK_BOX(hbox), self->entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), self->search_button, FALSE, FALSE, 0);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_add(GTK_CONTAINER(scrolled_window), self->text_view);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(window), vbox);
}

GType dictionary_panel_get_type(void)
{
	static GType type = 0;

	if (type == 0)
	{
		const GTypeInfo info = {
			sizeof (DictionaryPanelClass),
			NULL,
			NULL,
			(GClassInitFunc)dictionary_panel_class_init,
			NULL,
			NULL,
			sizeof(DictionaryPanel),
			0,
			(GInstanceInitFunc)dictionary_panel_init
		};

		type = g_type_register_static(GTK_TYPE_WINDOW, "Dictionary", &info, 0);
	}

	return type;
}

GtkWidget * dictionary_panel_new(void)
{
	DictionaryPanel * dict;

	dict = g_object_new(DICTIONARY_TYPE_PANEL, "type", GTK_WINDOW_TOPLEVEL, NULL);

	return GTK_WIDGET(dict);
}

int main(int ac, char ** av)
{
	GtkWidget * dict;
	gtk_init(&ac, &av);

	dict = dictionary_panel_new();

	gtk_widget_show_all(dict);
	gtk_main();

	return 0;
}
