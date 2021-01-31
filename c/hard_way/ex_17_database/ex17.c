#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define VERBOSE 0

struct Address {
  int id;
  int set;
  char *name;
  char *email;
};

struct Database {
  int max_data;
  int max_rows;
  struct Address *rows;
};

struct Connection {
  FILE *file;
  struct Database *db;
};

void Database_close(struct Connection *conn);

void die(const char *message, struct Connection *conn) {
  if (errno) {
    perror(message);
  } else {
    printf("ERROR: %s\n", message);
  }
 
  if (conn) {
    Database_close(conn);
  } else {
    printf("Attempted to close the database. CONNECTION NOT FOUND\n");
  }

  exit(1);
}

void Address_print(struct Address *addr) {
  printf("%d %s %s\n", addr->id, addr->name, addr->email);
}

void Database_load(struct Connection *conn) {
  int rc = fread(conn->db, sizeof(struct Database), 1, conn->file);
  if (rc != 1) {
    die("Failed to load database.", conn);
  }
}

struct Connection *Database_open(const char *filename, char mode) {
  struct Connection *conn = malloc(sizeof(struct Connection));
  if (!conn) {
    die("Memory error", conn);
  }

  conn->db = malloc(sizeof(struct Database));
  if (!conn->db) {
    die("Memory error", conn);
  }

  if (mode == 'c') {
    conn->file = fopen(filename, "w");
  } else {
    conn->file = fopen(filename, "r+");

    if (conn->file) {
      Database_load(conn);
    }
  }

  if (!conn->file) {
    die("Failed to open the file", conn);
  }

  return conn;
}

void Database_close(struct Connection *conn) {
  printf("Closing Database...\n");
  if (conn) {
    if (conn->file) {
      fclose(conn->file);
    }
    if (conn->db) {
      free(conn->db);
    }
    free(conn);
  }
}

void Database_write(struct Connection *conn) {
  rewind(conn->file);

  int rc = fwrite(conn->db, sizeof(struct Database), 1, conn->file);
  if (rc != 1) {
    die("Failed to write database.", conn);
  }

  rc = fflush(conn->file);
  if (rc == -1) {
    die("Cannot flush database.", conn);
  }
}

/* why exactly are we creating the whole database and fill it with empty structs?
 * are there better ways of 'putting aside' memory for later usage?
 * is it the only way to ensure write success? it can't be
 */
void Database_create(struct Connection *conn) {
  int i = 0;

  for (i = 0; i < conn->db->max_rows; i++) {
    // make a prototype to initialize it
    struct Address addr = { .id = i, .set = 0 };
    // then just assign it
    conn->db->rows[i] = addr;
  }
}

/* Sets a record in a database - in a way, it is the 'create' method, CRUD-wise
 * although since int `Database_create` I'm actually creating the 'slots'
 * for data, this is just setting.
 */
void Database_set(struct Connection *conn, int id, const char *name, const char *email) {
  // find the row, in the db, in the connection, and store its address ass `addr`
  struct Address *addr = &conn->db->rows[id];

  // since `addr` is a struct, I can look at the value of `set`
  if (addr->set) {
    die("Already set, delete it first", conn);
  }

  addr->set = 1;
  char *res = strncpy(addr->name, name, conn->db->max_data);
  // fix for buff overflow
  res[conn->db->max_data - 1] = '\0';
  if (!res) {
    die("Name copy failed", conn);
  }

  res = strncpy(addr->email, email, conn->db->max_data);
  // fix for buff overflow
  res[conn->db->max_data - 1] = '\0';
  if (!res) {
    die("Email copy failed", conn);
  }
}

/* performs a 'get' from the database. R in CRUD
 * and prints out the given field to the screen
 */ 
void Database_get(struct Connection *conn, int id) {
  struct Address *addr = &conn->db->rows[id];

  if (addr->set) {
    Address_print(addr);
  } else {
    die("ID is not set", conn);
  }
}

/* D in CRUD, removes a record by un-setting it */
void Database_delete(struct Connection *conn, int id) {
  // create an empty `addr` record
  struct Address addr = { .id = id, .set = 0 };
  // put it in where is the record that we want to delete
  conn->db->rows[id] = addr;
}

/* lists all the records in the database
 * simply by iterating over every address we've got there
 * and printing it if it's set.
 */
void Database_list(struct Connection *conn, int verbose) {
  int i = 0;
  struct Database *db = conn->db;

  for (i = 0; i < conn->db->max_rows; i++) {
    struct Address *cur = &db->rows[i];

    if (cur->set) {
      Address_print(cur);
    } else {
      if (verbose) printf("No record at %d\n", i);
    }
  }
}

int main(int argc, char *argv[]) {

  if (argc < 3) {
    die("USAGE: ex17 <dbfile> <action> [action params]", NULL);
  } 

  char *filename = argv[1];
  char action = argv[2][0];
  struct Connection *conn = Database_open(filename, action);
  int id = 0;

  if (argc > 3) id = atoi(argv[3]);
  if (id >= conn->db->max_rows) die("There's not that many records.", conn);

  switch(action) {
    case 'c':
      Database_create(conn);
      Database_write(conn);
      break;

    case 'g':
      if (argc != 4) {
        die("Need an id to get", conn);
      }
      Database_get(conn, id);
      break;

    case 's':
      if (argc != 6) {
        die("Need id, name, email to set", conn); 
      }

      Database_set(conn, id, argv[4], argv[5]);
      Database_write(conn);
      break;

    case 'd':
      if (argc != 4) {
        die("Need id to delete", conn);
      }

      Database_delete(conn, id);
      Database_write(conn);
      break;

    case 'l':
      Database_list(conn, VERBOSE);
      break;

    default:
      die("Invalid action. Try c=create, g=get, s=set, d=delete, l=list", conn);
  }

  Database_close(conn);

  return 0;
}
