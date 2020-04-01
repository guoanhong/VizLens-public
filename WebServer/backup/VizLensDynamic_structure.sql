CREATE TABLE "answers_crop" (
	`id`	INTEGER,
	`session`	TEXT,
	`stateid`	INTEGER,
	`complete`	INTEGER, -- 0: not answered; 1: complete; 2: incomplete
	`clear`	INTEGER, -- 0: not answered; 1: clear; 2: unclear
	`x1`	REAL,
	`y1`	REAL,
	`x2`	REAL,
	`y2`	REAL,
	`buttons`	INTEGER,
	`description`	TEXT,
	`starttime`	char(50),
	`endtime`	char(50),
	`source`	TEXT
);

CREATE TABLE "answers_label" (
	`id`	INTEGER,
	`session`	TEXT,
	`stateid`	INTEGER,
	`x1`	REAL,
	`y1`	REAL,
	`x2`	REAL,
	`y2`	REAL,
	`label`	TEXT,
	`starttime`	char(50),
	`endtime`	char(50),
	`button`	INTEGER,
	`source`	TEXT,
	`workerid`	TEXT,
	`active`	INTEGER
);

CREATE TABLE "images_original" (
	`id`	INTEGER PRIMARY KEY AUTOINCREMENT,
	`userid`	TEXT,
	`session`	TEXT,
	`stateid`	INTEGER,
	`image`	TEXT,
	`active`	INTEGER DEFAULT 1,
	`timestamp`	DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE "images_reference" (
	`id`	INTEGER PRIMARY KEY AUTOINCREMENT,
	`userid`	TEXT,
	`session`	TEXT,
	`stateid`	INTEGER,
	`image`	TEXT,
	`active`	INTEGER DEFAULT 1,
	`timestamp`	DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE `images_video` (
	`id`	INTEGER PRIMARY KEY AUTOINCREMENT,
	`userid`	TEXT,
	`session`	TEXT,
	`stateid`	INTEGER,
	`image`	TEXT,
	`imageactive`	INTEGER,
	`target`	TEXT,
	`uploadtime`	INTEGER DEFAULT CURRENT_TIMESTAMP,
	`feedback`	TEXT,
	`feedbackactive`	INTEGER,
	`resulttime`	INTEGER,
	`feedbacktime`	INTEGER,
	`objectfound`	INTEGER,
	`fingerfound`	INTEGER,
	`fingertipx`	INTEGER,
	`fingertipy`	INTEGER
);

CREATE TABLE "log_experiment" (
	`id`	INTEGER PRIMARY KEY AUTOINCREMENT,
	`userid`	TEXT,
	`button`	TEXT,
	`timestamp`	TEXT DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE `mturk_hits` (
	`session`	TEXT,
	`stateid`	INTEGER,
	`task`	TEXT,
	`info`	TEXT,
	`timestamp`	DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE "object_buttons" (
	`id`	INTEGER,
	`session`	TEXT,
	`stateid`	INTEGER,
	`x1`	REAL,
	`y1`	REAL,
	`x2`	REAL,
	`y2`	REAL,
	`label`	TEXT
);

CREATE TABLE `objects` (
	`id`	INTEGER PRIMARY KEY AUTOINCREMENT,
	`userid`	TEXT,
	`session`	TEXT,
	`stateid`	INTEGER,
	`image`	TEXT,
	`status`	TEXT,
	`description`	TEXT,
	`timestamp`	DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE "workers" (
	`imageid`	INTEGER,
	`workerid`	TEXT,
	`jointime`	DATETIME DEFAULT CURRENT_TIMESTAMP,
	`finishtime`	DATETIME,
	`session`	TEXT,
	`stateid`	INTEGER,
	`task`	TEXT,
	`startposition`	INTEGER
);