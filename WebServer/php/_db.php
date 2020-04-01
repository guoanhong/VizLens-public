<?php
	function getDatabaseHandle() {
		//echo("Connecting to DB...");
		$dbh = new PDO("sqlite:../db/VizLensDynamic.db");
		//echo("Got DB handle.");
		return $dbh;
	}
?>