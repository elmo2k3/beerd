
CREATE TABLE tags ( tag TEXT, user_id INTEGER,permission INTEGER);
CREATE TABLE users ( name TEXT, surname TEXT, nick TEXT, email TEXT, age INTEGER, weight INTEGER, size INTEGER, gender INTEGER, permission INTEGER, password TEXT, pic BLOB);
CREATE TABLE actions ( timestamp INTEGER, action_id INTEGER, action_value1 TEXT, action_value2 TEXT);
