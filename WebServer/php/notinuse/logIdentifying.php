<?php
    error_reporting(E_ALL);
    ini_set("display_errors", 1);

    include('_db.php');

    try {
        $dbh = getDatabaseHandle();
    } catch( PDOException $e ) {
        echo $e->getMessage();
    }

    if( $dbh ) {
        $userid = $_REQUEST["userid"];
        $button = $_REQUEST["button"];

        $sth = $dbh->prepare("INSERT INTO log_experiment (userid,button) VALUES(:userid,:button)");
        $sth->execute(array(':userid'=>$userid, ':button'=>$button));

        print("SUCCESS");
    }
    else {
        print("FAILING");
    }
?>