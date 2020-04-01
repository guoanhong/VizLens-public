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

        $sth = $dbh->prepare("SELECT DISTINCT session FROM objects WHERE userid=:userid AND status!='deprecated'");
        $sth->execute(array(':userid'=>$userid));
        $row = $sth->fetchAll();

        $object_list = "";
        for ($i=0; $i < count($row); $i++) { 
            $object_list = $object_list . $row[$i]['session'] . "|ready";
            // $object_list = $object_list . $row[$i]['session'] . "|" . $row[$i]['status'];
            if ($i < count($row) - 1) {
                $object_list = $object_list . ";";
            }
        }
        print($object_list);
    }
    else {
        print("FAILING");
    }
?>