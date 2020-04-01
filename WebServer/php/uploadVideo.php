<?php
    error_reporting(E_ALL);
    ini_set("display_errors", 1);

    include('_db.php');

    foreach ($_FILES["pictures"]["error"] as $key => $error) {
        if ($error == UPLOAD_ERR_OK) {
            $tmp_name = $_FILES["pictures"]["tmp_name"][$key];
            $name = $_FILES["pictures"]["name"][$key];

            $session = $_REQUEST["session"];
            if (!file_exists('../images_video/'.$session)) {
                mkdir("../images_video/$session", 0777);
            }
            move_uploaded_file($tmp_name, "../images_video/$session/$name");

            // Insert pointer into DB
            try {
                $dbh = getDatabaseHandle();
                error_log("Got database handle.");
            } catch( PDOException $e ) {
                error_log("Error: " . $e);
                echo $e->getMessage();
            }

            if( $dbh ) {
                $userid = $_REQUEST["userid"];
                $mode = $_REQUEST["mode"];
                $target = $_REQUEST["target"];

                // $dbh->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);

                $sth = $dbh->prepare("INSERT INTO images_video (userid,session,image,imageactive,target) VALUES(:userid, :session, :image, :imageactive, :target)");

                $sth->execute(array(':userid'=>'' . $userid, ':session'=>'' . $session, ':image'=>'' . $name, ':imageactive'=>$mode, ':target'=>$target));

                error_log("Successfully insert image.");

                $current_date = date('d/m/Y==H:i:s');

                print("SUCCESS");
                print($current_date);
            }
            else {
                error_log("Failing");
                print("FAILING");
            }
        }
    }
?>