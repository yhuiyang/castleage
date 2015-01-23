-- config table
CREATE TABLE config(
	email TEXT UNIQUE,
	password TEXT,
	duel_higher INTEGER,
	duel_lower INTEGER
);

-- enemy bgh table
CREATE TABLE arena_enemy_duel_data(
	player_fbid TEXT NOT NULL,
	enemy_fbid TEXT NOT NULL,
	bqh TEXT NOT NULL,
	timestamp INTEGER NOT NULL
);


-- arena duel result 
CREATE TABLE arena_duel_result(
	player_fbid TEXT NOT NULL,
	enemy_fbid TEXT NOT NULL,
	win INTEGER DEFAULT 0,
	lose INTEGER DEFAULT 0,
	UNIQUE(player_fbid, enemy_fbid)
);

