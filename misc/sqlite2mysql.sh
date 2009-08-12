#!/bin/bash
USER=beer_admin
PASS=fooblabar
#PASS=beer_admin
DB=beer
HOST="87.76.20.85"

echo "DROP TABLE tags" | mysql -u $USER --password=$PASS $DB -h $HOST
echo "DROP TABLE users" | mysql -u $USER --password=$PASS $DB -h $HOST
echo "DROP TABLE actions" | mysql -u $USER --password=$PASS $DB -h $HOST

sqlite3 $1 .dump | egrep -vi '^(BEGIN TRANSACTION|COMMIT)' | perl -pe 's/INSERT INTO \"(.*)\" VALUES/INSERT INTO \1 VALUES/' | mysql -u $USER --password=$PASS $DB -h $HOST
