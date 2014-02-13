
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "newdirs.h"

enum {
	NEWDIRS_SIZE = 1024,
	MULTIPLIER   = 31,
};


/* stores parent dirs of newdirs */
parent_t *parents[NEWDIRS_SIZE];

/*
 * hashtable hash function
 */
uint64_t
hash(const char *path) {
	uint64_t h;
	unsigned char *p;

	h = 0;
	for (p = (unsigned char *)path; *p != '\0'; p++)
		h = (MULTIPLIER * h + *p) % NEWDIRS_SIZE;
	return h;
}

/*
 * dirname version that allocates a new buffer
 */
char *
dirname(const char *path) {
	char *dirname, *p;
	size_t len;
	
	p = strrchr(path, '/');
	if (!p) {
		len = 1;
		path = ".";
	} else if (p == path) {
		len = 1;
		path = "/";
	} else {
		len = p - path;
	}
	dirname = malloc(len+1);
	strncpy(dirname, path, len);
	dirname[len] = '\0';
	return dirname;
}

/*
 * basename version that allocates a new buffer
 */
char *
basename(const char *path) {
	const char *p;

	p = strrchr(path, '/');
	if (!p)
		p = path;
	else
		p++;
	return strdup(p);
}

/*
 * lookup parent directory in hashtable
 */
parent_t *
plookup(const char *path, uint64_t h, char create) {
	parent_t *p;

	for (p = parents[h]; p != NULL; p = p->next) {
		if (strcmp(p->path, path) == 0)
			return p;
	}
	if (create) {
		printf("creating %s with %llu\n", path, h);
		p = malloc(sizeof(parent_t));
		p->path = strdup(path);
		p->prev = NULL;
		p->next = parents[h];
		p->children = NULL;
		if (p->next) p->next->prev = p;
		parents[h] = p;
		return p;
	}
	return NULL;
}

/*
 * lookup entry in parent directory by name
 */
entry_t *
elookup(parent_t *parent, char *name, char create) {
	entry_t *e;

	for (e = parent->children; e != NULL; e = e->next) {
		if (strcmp(e->name, name) == 0)
			return e;
	}
	if (create) {
		e = malloc(sizeof(entry_t));
		e->name = strdup(name);
		e->prev = NULL;
		e->next = parent->children;
		parent->children = e;
		if (e->next) e->next->prev = e;
		return e;
	}
	return NULL;
}

void
newdirs_add(const char *path) {
	uint64_t h;
	char *ppath, *ename;
	parent_t *parent;

	ppath = dirname(path);
	ename = basename(path);
	printf("newdirs_add: ppath: %s, ename: %s\n", ppath, ename);
	h = hash(ppath);
	parent = plookup(ppath, h, 1);
	printf("newdirs_add: created parent: %s\n", parent->path);
	elookup(parent, ename, 1);
	printf("newdirs_add: created entry: %s\n", parent->children->name);

	free(ppath);
	free(ename);
}

void
newdirs_remove(const char *path) {
	uint64_t h;
	char *ppath, *ename;
	parent_t *p;
	entry_t *e;

	ppath = dirname(path);
	ename = basename(path);
	h = hash(ppath);
	p = plookup(ppath, h, 0);
	if (!p) goto clean;

	e = elookup(p, ename, 0);
	if (e) {
		if (e->prev) e->prev->next = e->next;
		if (e->next) e->next->prev = e->prev;
		if (p->children == e) p->children = e->next;
		free(e->name);
		free(e);
	}
	if (!p->children) {
		parents[h] = p->next;
		if (p->next) p->next->prev = p->prev;
		if (p->prev) p->prev->next = p->next;
		free(p->path);
		free(p);
	}

clean:
	free(ppath);
	free(ename);
}

char
newdirs_exists(const char *path) {
	parent_t *p;
	entry_t *e;
	char *ppath, *ename;
	uint64_t h;

	e = NULL;
	ppath = dirname(path);
	ename = basename(path);
	printf("newdirs_exists: ppath = %s, ename = %s\n", ppath, ename);
	h = hash(ppath);
	p = plookup(ppath, h, 0);
	printf("newdirs_exists: parent path = %p\n", p);
	if (p)
		e = elookup(p, ename, 0);
	printf("newdirs_exists: e = %p\n", e);
	free(ppath);
	free(ename);
	return (e != NULL);
}

parent_t *
newdirs_list(const char *path) {
	uint64_t h;

	h = hash(path);
	printf("newdirs_list: %s\n", path);
	return plookup(path, h, 0);
}

