#include "dhdb_xml.h"

static void test_xml()
{
	dhdb_t *s = dhdb_create();
	dhdb_t *body = dhdb_create();
	dhdb_set_obj_str(body, "body", "Body content");
	dhdb_t *html = dhdb_create();
	dhdb_t *head = dhdb_create();
	dhdb_t *title = dhdb_create();
	dhdb_set_obj(html, "html", body);
	dhdb_set_obj_str(title, "title", "Title content");
	dhdb_set_obj(head, "head", title);

	dhdb_add(s, html);
	dhdb_add(html, head);

	dhdb_free(s);
}

int main(int argc, char **argv)
{
	test_xml();
}
