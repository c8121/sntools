/*
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
 */

/*
 * Author: christian c8121 de
 */

#include "lmdb.h"

char *mdbDataDir = "/var/spool/sntools";
mode_t mdbDataMode = 0774;

MDB_env *mdbEnv;
MDB_txn *mdbTxn;
MDB_dbi mdbDbi;



/**
 * Create LMDB resources
 * @return 0 on success
 */
int prepareMdb() {

	struct stat fileStat;
	if( stat(mdbDataDir, &fileStat) != 0 ) {
		printf("Create data directory: %s\n", mdbDataDir);
		if( mkdir(mdbDataDir, mdbDataMode) != 0 ) {
			fprintf(stderr, "Failed to create %s\n", mdbDataDir);
			return 1;
		}
	}

	if( mdb_env_create(&mdbEnv) != MDB_SUCCESS ) {
		fprintf(stderr, "Failed to create LMDB environment\n");
		return 1;
	}

	if( mdb_env_open(mdbEnv, mdbDataDir, 0, mdbDataMode) != MDB_SUCCESS ) {
		mdb_env_close(mdbEnv);
		fprintf(stderr, "Failed to open LMDB environment\n");
		return 1;
	}

	if( mdb_txn_begin(mdbEnv, NULL, 0, &mdbTxn) != MDB_SUCCESS ) {
		mdb_env_close(mdbEnv);
		fprintf(stderr, "Failed to begin LMDB transaction\n");
		return 1;
	}

	if( mdb_dbi_open(mdbTxn, NULL, 0, &mdbDbi) != MDB_SUCCESS ) {
		mdb_txn_abort(mdbTxn);
		mdb_env_close(mdbEnv);
		fprintf(stderr, "Failed to open LMDB database\n");
		return 1;
	}

	return 0;

}

/**
 *
 */
void closeMdb() {

	if( mdb_txn_commit(mdbTxn) != MDB_SUCCESS ) {
		fprintf(stderr, "Failed to commit LMDB transaction\n");
	}

	mdb_dbi_close(mdbEnv, mdbDbi);
	mdb_env_close(mdbEnv);

}

/**
 *
 */
int getMdb(char *key, MDB_val *value) {

	MDB_val key_val;
	key_val.mv_size = strlen(key)+1;
	key_val.mv_data = key;

	return mdb_get(mdbTxn, mdbDbi, &key_val, value);
}

/**
 *
 */
void putMdb(char *key, char *value) {

	MDB_val key_val;
	key_val.mv_size = strlen(key)+1;
	key_val.mv_data = key;

	MDB_val data_val;
	data_val.mv_size = strlen(value)+1;
	data_val.mv_data = value;

	if( mdb_put(mdbTxn, mdbDbi, &key_val, &data_val, 0) != MDB_SUCCESS ) {
		fprintf(stderr, "Failed to store key %s", key);
	}
}
