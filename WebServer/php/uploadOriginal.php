<?php
    error_reporting(E_ALL);
    ini_set("display_errors", 1);

    include('_db.php');

    foreach ($_FILES["pictures"]["error"] as $key => $error) {
        if ($error == UPLOAD_ERR_OK) {
            $userid = $_REQUEST["userid"];
            $session = $_REQUEST["session"];
            $stateid = $_REQUEST["stateid"];

            $tmp_name = $_FILES["pictures"]["tmp_name"][$key];
            $name = $_FILES["pictures"]["name"][$key];
            if (!file_exists('../images_original/'.$session)) {
                mkdir("../images_original/$session", 0777);
            }
            move_uploaded_file($tmp_name, "../images_original/$session/$name");

            // $filename = '../images_original/$name';
            // $degrees = 90;

            // // Content type
            // header('Content-type: image/jpeg');

            // // Load
            // $source = imagecreatefromjpeg($filename);

            // // Rotate
            // $rotate = imagerotate($source, $degrees, 0);

            // // Output
            // imagejpeg($rotate);

            // // Free the memory
            // imagedestroy($source);
            // imagedestroy($rotate);

            // Insert pointer into DB
            try {
                $dbh = getDatabaseHandle();
            } catch( PDOException $e ) {
                echo $e->getMessage();
            }

            if( $dbh ) {
                // if not exist, insert; if exist, archive and add new.
                $query = $dbh->prepare("SELECT id FROM images_original WHERE session=:session AND stateid=:stateid AND active=1");
                $query->execute(array(':session'=>$session, ':stateid'=>$stateid));
                $row_check = $query->fetchAll();

                if (count($row_check) != 0) {
                    // archive in original
                    $query = $dbh->prepare("UPDATE images_original SET active=:active WHERE session=:session AND stateid=:stateid");
                    $query->execute(array(':active'=>-1, ':session'=>$session, ':stateid'=>$stateid));

                    // archive in objects
                    $query = $dbh->prepare("UPDATE objects SET status='deprecated' WHERE session=:session AND stateid=:stateid");
                    $query->execute(array(':session'=>$session, ':stateid'=>$stateid));

                    // archive in answers_crop 
                    $query = $dbh->prepare("UPDATE answers_crop SET source='deprecated' WHERE session=:session AND stateid=:stateid");
                    $query->execute(array(':session'=>$session, ':stateid'=>$stateid));
                }

                $query = $dbh->prepare("INSERT INTO images_original (userid,session,stateid,image,active) VALUES(:userid,:session,:stateid,:image,:active)");
                $query->execute(array(':userid'=>$userid, ':session'=>$session, ':stateid'=>$stateid, ':image'=>$name, ':active'=>1));

                $query = $dbh->prepare("INSERT INTO objects (userid,session,stateid,image,status) VALUES(:userid,:session,:stateid,:image,:status)");
                $query->execute(array(':userid'=>$userid, ':session'=>$session, ':stateid'=>$stateid, ':image'=>$name, ':status'=>'processing'));

                print("SUCCESS");
            }
            else {
                print("FAILING");
            }

            $command = escapeshellcmd('python ../mturk/createExternalHIT-crop.py '.$session.' '.$stateid);
            $output = shell_exec($command);
            print($output);
            $sth_HITs = $dbh->prepare("INSERT INTO mturk_hits (session,stateid,task,info) VALUES(:session,:stateid,:task,:info)");
            $sth_HITs->execute(array(':session'=>$session, ':stateid'=>$stateid, ':task'=>'crop', ':info'=>'fired hit ' . $output));
        }
    }
?>