<?php
    error_reporting(E_ALL);
    ini_set("display_errors", 1);

    //include('../../common_php/getDB.php');
    include('../php/_db.php');

    if( !isset($_REQUEST['session']) ) {
        print 'no session info';
    }

    if( isset($_REQUEST['session']) ) {
        try {
            $dbh = getDatabaseHandle();
        } catch( PDOException $e ) {
            echo $e->getMessage();
        }

        if( $dbh ) {
            $session  = $_REQUEST['session'];

            // Query to find segment information
            $sth = $dbh->prepare("SELECT x1, y1, x2, y2, label FROM answers WHERE session=:session");
            $sth->execute(array(':session'=>$session));

            $retStr = "{";
            // Resolution
            $retStr = $retStr."\"resolution\":[1280,960],\"box\":[";
            // Loop over entries and return info as a set of all to-filter segments!
            while( $row = $sth->fetch(PDO::FETCH_ASSOC, PDO::FETCH_ORI_NEXT) ) {
                $posx1 = intval($row["x1"]);
                $posy1 = intval($row["y1"]);
                $posx2 = intval($row["x2"]);
                $posy2 = intval($row["y2"]);
                $label = $row["label"];

                $div = "";
                if( $retStr <> "" ) {
                    $div = ",";
                }

                // Add to filter set
                $retStr = $retStr . "[" . $posx1 . "," . $posy1 . "," . $posx2 . "," . $posy2 . ",\"" . $label . "\"],";
            }

            $retStr = substr($retStr, 0 , -1);
            $retStr = $retStr."]}";
            $length = strlen($retStr);
            //$retStr[$length-3] = '';
            // Return the results
            print($retStr);
        }
        else {
            print("FAILING");
        }
    }
?>
