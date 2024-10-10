#include <sqlite3.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "../config/env.h"
#include "./db.h"

// Function to initialize the database
void init_db() {
    sqlite3 *db;
    char *err_msg = 0;
    int rc;

    printf("Initializing database...\n");


    // Open the database (will create if it doesn't exist)
    rc = sqlite3_open(db_path, &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    // SQL command to create xid table if it doesn't exist
    const char *sql_xid_table =
        "CREATE TABLE IF NOT EXISTS xid_table("
        "id INTEGER PRIMARY KEY, "
        "xid INTEGER NOT NULL);";

    // Execute the query to create xid table
    rc = sqlite3_exec(db, sql_xid_table, 0, 0, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to create xid_table: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        exit(1);
    }

    // SQL command to create IP lease table if it doesn't exist
    const char *sql_ip_lease_table =
        "CREATE TABLE IF NOT EXISTS lease_table("
        "id INTEGER PRIMARY KEY, "
        "ip_address TEXT NOT NULL, "
        "mac_address TEXT NOT NULL, "
        "lease_time INTEGER NOT NULL);";

    // Execute the query to create the lease table
    rc = sqlite3_exec(db, sql_ip_lease_table, 0, 0, &err_msg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to create lease_table: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        exit(1);
    }

    printf("Database initialized successfully.\n");

    sqlite3_close(db);
}