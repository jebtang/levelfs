
#include <stdlib.h>
#include <string.h>

#include "db.h"

levelfs_db_t *
levelfs_db_open(const char *path, char **errptr) {
	levelfs_db_t *out;
	leveldb_options_t *opts;
	leveldb_t *db;

	opts = leveldb_options_create();
	leveldb_options_set_create_if_missing(opts, 1);
	db = leveldb_open(opts, path, errptr);
	if (*errptr) {
		leveldb_options_destroy(opts);
		return NULL;
	}

	out = malloc(sizeof(levelfs_db_t));
	*out = (levelfs_db_t){ .db=db, .opts=opts };
	return out;
}

void
levelfs_db_close(levelfs_db_t *db) {
	leveldb_options_destroy(db->opts);
	leveldb_close(db->db);
	free(db);
}

const char *
levelfs_db_get(levelfs_db_t *db, const char *key, size_t klen,
               size_t *vlen, char **errptr) {
	const char *val;
	leveldb_readoptions_t *opts;

	opts = leveldb_readoptions_create();
	val = leveldb_get(db->db, opts, key, klen, vlen, errptr);
	leveldb_readoptions_destroy(opts);

	return val;
}

levelfs_iter_t *
levelfs_iter_seek(levelfs_db_t *db, const char *key, size_t klen) {
	levelfs_iter_t *it;
	leveldb_readoptions_t *opts;

	it = malloc(sizeof(levelfs_iter_t));
	memset(it, 0, sizeof(levelfs_iter_t));

	it->base_key_len = klen;
	it->base_key = malloc(klen);
	memcpy(it->base_key, key, klen);

	opts = leveldb_readoptions_create();
	it->it = leveldb_create_iterator(db->db, opts);
	leveldb_iter_seek(it->it, it->base_key, it->base_key_len);
	it->first = 1;

	return it;
}

const char *
levelfs_iter_next(levelfs_iter_t *it, size_t *klen) {
	const char *next_key = NULL;
	*klen = 0;

	if (it->first) {
		next_key = leveldb_iter_key(it->it, klen);
		it->first = 0;
	} else {
		leveldb_iter_next(it->it);
		if (leveldb_iter_valid(it->it))
			next_key = leveldb_iter_key(it->it, klen);
	}

	if (it->base_key_len > *klen) {
		klen = 0;
		return NULL;
	}
	if (strncmp(it->base_key, next_key, it->base_key_len) != 0) {
		klen = 0;
		return NULL;
	}

	return next_key;
}

const char *
levelfs_iter_value(levelfs_iter_t *it, size_t *vlen) {
	return leveldb_iter_value(it->it, vlen);
}

void
levelfs_iter_close(levelfs_iter_t *it) {
	if (it->it) leveldb_iter_destroy(it->it);
	if (it->opts) leveldb_readoptions_destroy(it->opts);
	if (it->base_key) free(it->base_key);
	free(it);
}
