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

        // For testing without CVServer
        // $stateid = 0;

        $sth = $dbh->prepare("SELECT x1, y1, x2, y2, buttons, description FROM answers_crop WHERE session=:session AND stateid=:stateid AND source=:source ORDER BY RANDOM() ASC LIMIT 1");
        $sth->execute(array(':session'=>$object, ':stateid'=>$stateid, ':source'=>"aggregated"));
        $refImage = $sth->fetch(PDO::FETCH_ASSOC, PDO::FETCH_ORI_NEXT);
        $width = $refImage["x2"] - $refImage["x1"];
        $height = $refImage["y2"] - $refImage["y1"];

        error_log("Interface width: " . $width);
        error_log("Interface height: " . $height);

        if ($refImage != null && $width != 0 && $height != 0) {
            if ($width > $height) {
                $height = round($height / $width, 3);
                $buttons = $refImage["buttons"];
                $description = $refImage["description"];
                $row = array();
                array_push($row, array('width' => "1.000", 'height' => "" . $height, 'buttons' => $buttons, 'description' => $description));

                $sth = $dbh->prepare("SELECT label, x1, y1, x2, y2 FROM object_buttons WHERE session=:session AND stateid=:stateid");
                $sth->execute(array(':session'=>$object, ':stateid'=>$stateid));
                $button_rows = $sth->fetchAll(PDO::FETCH_ASSOC);

                for ($i=0; $i < count($button_rows); $i++) { 
                    array_push($row, array('label' => $button_rows[$i]['label'], 
                        'x1' => "" . round($button_rows[$i]['x1'] / $width, 3), 
                        'y1' => "" . round($button_rows[$i]['y1'] / $width, 3),
                        'x2' => "" . round($button_rows[$i]['x2'] / $width, 3), 
                        'y2' => "" . round($button_rows[$i]['y2'] / $width, 3)));
                }
            } else {
                $width = round($width / $height, 3);
                $buttons = $refImage["buttons"];
                $description = $refImage["description"];
                $row = array();
                array_push($row, array('width' => "" . $width, 'height' => "1.000", 'buttons' => $buttons, 'description' => $description));

                $sth = $dbh->prepare("SELECT label, x1, y1, x2, y2 FROM object_buttons WHERE session=:session AND stateid=:stateid");
                $sth->execute(array(':session'=>$object, ':stateid'=>$stateid));
                $button_rows = $sth->fetchAll(PDO::FETCH_ASSOC);

                for ($i=0; $i < count($button_rows); $i++) { 
                    array_push($row, array('label' => $button_rows[$i]['label'], 
                        'x1' => "" . round($button_rows[$i]['x1'] / $height, 3), 
                        'y1' => "" . round($button_rows[$i]['y1'] / $height, 3),
                        'x2' => "" . round($button_rows[$i]['x2'] / $height, 3), 
                        'y2' => "" . round($button_rows[$i]['y2'] / $height, 3)));
                }
            }

            $arrayData = array_map('array_values', array_values($row));

            print(json_encode($arrayData));
        }
        
    }
    else {
        print("FAILING");
    }
?>