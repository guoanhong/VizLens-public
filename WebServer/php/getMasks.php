<?php
    error_reporting(E_ALL);
    ini_set("display_errors", 1);
    
    include('../php/_db.php');

    if( !isset($_REQUEST['session']) ) {
        print 'no session info';
    }

    if( !isset($_REQUEST['stateid']) ) {
        print 'no stateid info';
    }

    if( isset($_REQUEST['session']) && isset($_REQUEST['stateid']) ) {
        try {
            $dbh = getDatabaseHandle();
        } catch( PDOException $e ) {
            echo $e->getMessage();
        }

        if( $dbh ) {
            $session  = $_REQUEST['session'];
            $stateid  = $_REQUEST['stateid'];

            // use Worker Id to determine what to show and what not to show
            $workerid  = $_REQUEST['workerid'];

            $retStr = "";

            // Query to find segment information
            $sth = $dbh->prepare("SELECT x1, y1, x2, y2, label FROM answers_label WHERE session=:session AND stateid=:stateid AND source=:source");
            $sth->execute(array(':session'=>$session, ':stateid'=>$stateid, ':source'=>'individual'));

            // Resolution
            //$retStr = $retStr."\"resolution\":[1280,960],\"box\":[";
            // Loop over entries and return info as a set of all to-filter segments!
            while( $row = $sth->fetch(PDO::FETCH_ASSOC, PDO::FETCH_ORI_NEXT) ) {
                $posx1 = intval($row["x1"]);
                $posy1 = intval($row["y1"]);
                $posx2 = intval($row["x2"]);
                $posy2 = intval($row["y2"]);
                $label = $row["label"];

                $div = "";
                if( $retStr <> "" ) {
                    $div = ";";
                }

                // Add to filter set
                $retStr = $retStr . $div . $posx1 . "," . $posy1 . "," . $posx2 . "," . $posy2 . "," . $label;
            }
            $retStr = $retStr;
            //$length = strlen($retStr);
            //$retStr[$length-2] = "";
            // Return the results
            print($retStr);
        }
        else {
            print("FAILING");
        }
    }
?>
