# Castle Age

The purpose of scripts in this project is for education use. It helps
user play castle age arena ix duel easily.

This is the requirement to use this script:
- Crusaders of castle age. Means that you can access CA via web3/web4.
- sqlite3 command line tool, or any other GUI SQLite editors.
- python modules, requests, lxml, cssseelct...etc.


## How to use the script?

### Create database file used by script.
	$ sqlite3 castleage.sqlite
	sqlite> .read gen_table.sql
	sqlite> INSERT INTO config VALUES('YourEmail', 'YourPassword', 0, 0);

It is allowed to create multiple accounts.

### Run it.
	$ python main.py

Enjoy it!
