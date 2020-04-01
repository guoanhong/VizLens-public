<?php
    error_reporting(E_ALL);
    ini_set("display_errors", 1);

    function setReference($dbh, $session, $stateid){
        //crop image, store into 
        // call uploadReference.php - upload image, update database, createHIT in uploadReference

        //read image from database
        $query = $dbh->prepare("SELECT * FROM images_original WHERE session=:session AND stateid=:stateid AND active=1 ORDER BY RANDOM() ASC LIMIT 1");
        $query->execute(array(':session'=>$session, ':stateid'=>$stateid));
        $row = $query->fetch(PDO::FETCH_ASSOC, PDO::FETCH_ORI_NEXT);

        $originalImagePath = "../images_original/" . $row["session"] . "/" . $row["image"];

        error_log("Original Image Path: " . $originalImagePath);
        error_log(json_encode($row));

        if (strlen($stateid) == 1) {
            $stateid = '0' . $stateid;
        }
        
        $referenceImageName = $stateid . "-crop.jpg";
        $referenceImagePath = "../images_reference/" . $session . "/" . $stateid . "-crop.jpg";
        if (!file_exists('../images_reference/'.$session)) {
            mkdir("../images_reference/$session", 0777);
        }

        $query = $dbh->prepare("SELECT x1, y1, x2, y2, buttons, description FROM answers_crop WHERE session=:session AND stateid=:stateid AND source=:source");
        $query->execute(array(':session'=>$session, ':stateid'=>$stateid, ':source'=>'aggregated'));
        $rowCrop = $query->fetch(PDO::FETCH_ASSOC, PDO::FETCH_ORI_NEXT);

        $posx1 = intval($rowCrop["x1"]);
        $posy1 = intval($rowCrop["y1"]);
        $posx2 = intval($rowCrop["x2"]);
        $posy2 = intval($rowCrop["y2"]);
        $buttons = $rowCrop["buttons"];
        $description = $rowCrop["description"];

        $width = $posx2 - $posx1;
        $height = $posy2 - $posy1;

        error_log($posx1 . " " . $posy1 . " " . $posx2 . " " . $posy2 . " " . $buttons . " " . $description);

        // Update description
        $query = $dbh->prepare("UPDATE objects SET description=:description WHERE session=:session AND stateid=:stateid AND status!='deprecated'");
        $query->execute(array(':description'=>$description, ':session'=>$session, ':stateid'=>$stateid));

        $src = imagecreatefromjpeg($originalImagePath);
        $dest = imagecreatetruecolor($width, $height);

        //crop image
        imagecopy($dest, $src, 0, 0, $posx1, $posy1, $width, $height);

        //store into folder
        imagejpeg($dest, $referenceImagePath);

        imagedestroy($dest);
        imagedestroy($src);

        //store into database
        $userid = $row["userid"];

        $query = $dbh->prepare("INSERT INTO images_reference (userid,session,stateid,image) VALUES(:userid,:session,:stateid,:image)");
        $query->execute(array(':userid'=>$userid, ':session'=>$session, ':stateid'=>$stateid, ':image'=>$referenceImageName));

        $HITs = round($buttons*2);

        // post HIT
        $command = escapeshellcmd('python ../mturk/createExternalHIT-label.py '.$session.' '.$stateid.' '.$HITs);
        $output = shell_exec($command);
        print($output);
        $sth_HITs = $dbh->prepare("INSERT INTO mturk_hits (session,stateid,task,info) VALUES(:session,:stateid,:task,:info)");
        $sth_HITs->execute(array(':session'=>$session, ':stateid'=>$stateid, ':task'=>'label', ':info'=>'fire ' . $HITs . ' hits ' . $output));
    }
?>