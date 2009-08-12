#!/bin/bash
USER=beer_admin
PASS=beer_admin
DB=beer

echo "DROP TABLE tags" | mysql -u $USER --password=$PASS $DB
echo "DROP TABLE users" | mysql -u $USER --password=$PASS $DB
echo "DROP TABLE actions" | mysql -u $USER --password=$PASS $DB

sqlite3 $1 .dump | egrep -vi '^(BEGIN TRANSACTION|COMMIT)' | perl -pe 's/INSERT INTO \"(.*)\" VALUES/INSERT INTO \1 VALUES/' | mysql -u $USER --password=$PASS $DB
