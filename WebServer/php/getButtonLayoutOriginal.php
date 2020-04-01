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

        $sth = $dbh->prepare("SELECT x1, y1, x2, y2, buttons FROM answers_crop WHERE session=:session AND source=:source ORDER BY RANDOM() ASC LIMIT 1");
        $sth->execute(array(':session'=>$object, ':source'=>"aggregated"));
        $refImage = $sth->fetch(PDO::FETCH_ASSOC, PDO::FETCH_ORI_NEXT);
        $width = $refImage["x2"] - $refImage["x1"];
        $height = $refImage["y2"] - $refImage["y1"];

        $ratio = 69 / $width;
        $width = $ratio * $width;
        $height = $ratio * $height;

        $buttons = $refImage["buttons"];

        $row = array();

        array_push($row, array('width' => "" . $width, 'height' => "" . $height));

        $sth = $dbh->prepare("SELECT label, x1, y1, x2, y2 FROM object_buttons WHERE session=:session");
        $sth->execute(array(':session'=>$object));

        $button_rows = $sth->fetchAll(PDO::FETCH_ASSOC);

        $button_size_total = 0;

        for ($i=0; $i < count($button_rows); $i++) {
            $button_size_total += ($button_rows[$i]['x2'] - $button_rows[$i]['x1'] + $button_rows[$i]['y2'] - $button_rows[$i]['y1']) * $ratio / 2;

            array_push($row, array('label' => $button_rows[$i]['label'], 
                'x1' => "" . $button_rows[$i]['x1'] * $ratio, 
                'y1' => "" . $button_rows[$i]['y1'] * $ratio,
                'x2' => "" . $button_rows[$i]['x2'] * $ratio, 
                'y2' => "" . $button_rows[$i]['y2'] * $ratio));
        }

        $button_size_avg = $button_size_total / count($button_rows);

        $row[0][2] = "" . $button_size_avg;

        $arrayData = array_map('array_values', array_values($row));

        print(json_encode($arrayData));
    }
    else {
        print("FAILING");
    }
?>