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
        $object = $_REQUEST["object"];
        $stateid = $_REQUEST["state"];

        $sth = $dbh->prepare("SELECT label FROM object_buttons WHERE session=:session AND stateid=:stateid");
        $sth->execute(array(':session'=>$object, ':stateid'=>$stateid));
        $row = $sth->fetchAll();

        $button_list = "";
        for ($i=0; $i < count($row); $i++) { 
            $button_list = $button_list . $row[$i]['label'];
            if ($i < count($row) - 1) {
                $button_list = $button_list . ";";
            }
        }
        print($button_list);
    }
    else {
        print("FAILING");
    }
?>