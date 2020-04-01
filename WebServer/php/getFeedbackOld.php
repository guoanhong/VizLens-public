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

        // search in images_video the latest feedbackactive = 1, and calculate feedback, and set to 0
        $sth_video = $dbh->prepare("SELECT session, refx, refy, feedback FROM images_video WHERE userid=:userid AND imageactive=:imageactive AND feedbackactive=:feedbackactive ORDER BY session");
        $sth_video->execute(array(':userid'=>$userid, ':imageactive'=>0, ':feedbackactive'=>1));

        $row = $sth_video->fetch(PDO::FETCH_ASSOC, PDO::FETCH_ORI_NEXT);

        // weird, but it's 4 when there is something.
        if (count($row) > 1) {
            // get buttons information in answers_label
            $sth_answer = $dbh->prepare("SELECT x1, y1, x2, y2, label FROM answers_label WHERE session=:session AND source=:source");
            // change session7
            $sth_answer->execute(array(':session'=>'office', ':source'=>'aggregated'));
            $row_answer = $sth_answer->fetchAll();

            $buttons_array = array();
            $button_height = 0;
            $button_width = 0;

            for ($i=0; $i < count($row_answer); $i++) { 
                $button_array = array();

                array_push($button_array, $row_answer[$i]['x1']);
                array_push($button_array, $row_answer[$i]['y1']);
                array_push($button_array, $row_answer[$i]['x2']);
                array_push($button_array, $row_answer[$i]['y2']);
                array_push($button_array, $row_answer[$i]['label']);
                array_push($button_array, ($row_answer[$i]['x1'] + $row_answer[$i]['x2'])/2);
                array_push($button_array, ($row_answer[$i]['y1'] + $row_answer[$i]['y2'])/2);

                $button_width += $row_answer[$i]['x2'] - $row_answer[$i]['x1'];
                $button_height += $row_answer[$i]['y2'] - $row_answer[$i]['y1'];

                array_push($buttons_array, $button_array);
            }

            $button_height = $button_height / count($row_answer);
            $button_width = $button_width / count($row_answer);

            // print(json_encode($buttons_array));
            // print("Button Height: " . $button_height . "\n");
            // print("Button Width: " . $button_width . "\n");
             
            $session = $row["session"];
            $x = $row["refx"];
            $y = $row["refy"] + $button_height/2;
            $feedback = $row["feedback"];

            // print("Current location: " . $x . " " . $y . " " . $feedback . "\n");

            $feedbacklabel = "";

            // change to $feedback
            if (1) {
                $flag = 0;
                $within_array = array();
                $offset_array = array();
                for ($i=0; $i < count($buttons_array); $i++) { 
                    if ($x > $buttons_array[$i][0] && $x < $buttons_array[$i][2] && $y > $buttons_array[$i][1] && $y < $buttons_array[$i][3]) {
                        array_push($within_array, 1);
                        // inside
                        $feedbacklabel = $buttons_array[$i][4];
                        // print("Feedback Label: " . $buttons_array[$i][4] . "\n");
                        $flag = 1;
                    } else {
                        array_push($within_array, 0);
                    }
                    $distance_array = array();
                    array_push($distance_array, sqrt(pow($x - $buttons_array[$i][5], 2) + pow($y - $buttons_array[$i][6], 2)));
                    array_push($distance_array, $x - $buttons_array[$i][5]);
                    array_push($distance_array, $y - $buttons_array[$i][6]);
                    array_push($distance_array, $buttons_array[$i][4]);
                    array_push($offset_array, $distance_array);
                }

                // change this to $flag == 0
                if ($flag == 0) {
                    $shortest_dist = 10000;
                    $shortest_index = 0;
                    $shortest2_dist = 10000;
                    $shortest2_index = 0;
                    for ($i=0; $i < count($buttons_array); $i++) {
                        $current_dist = $offset_array[$i][0];
                        if ($current_dist < $shortest_dist) {
                            $shortest2_dist = $shortest_dist;
                            $shortest2_index = $shortest_index;
                            $shortest_dist = $current_dist;
                            $shortest_index = $i;
                        } else if ($current_dist < $shortest2_dist) {
                            $shortest2_dist = $current_dist;
                            $shortest2_index = $i;
                        }
                    }

                    // print("Shortest Dist: " . $shortest_dist . "\n");
                    // print("Shortest Index: " . $shortest_index . "\n");
                    // print("Shortest2 Dist: " . $shortest2_dist . "\n");
                    // print("Shortest2 Index: " . $shortest2_index . "\n");
                     
                    // print(abs($offset_array[$shortest_index][1]) . "\n");
                    // print(abs($offset_array[$shortest2_index][1]) . "\n");
                    // print(abs($offset_array[$shortest_index][2]) . "\n");
                    // print(abs($offset_array[$shortest2_index][2]) . "\n");

                    if ( abs($offset_array[$shortest_index][1]) < $button_width*2 && abs($offset_array[$shortest2_index][1]) < $button_width*2 && abs($offset_array[$shortest_index][2]) < $button_height*2 && abs($offset_array[$shortest2_index][2]) < $button_height*2 ) {
                        $feedbacklabel = $offset_array[$shortest_index][3] . " and " . $offset_array[$shortest2_index][3];
                        // print("Feedback Label: " . $offset_array[$shortest_index][3] . " and " . $offset_array[$shortest2_index][3] . "\n");
                    } else {
                        $feedbacklabel = "nothing";
                        // print("PHP: No feedback.\n");
                    }
                }
            } else {
                $feedbacklabel = "";
                // print("C++: No feedback.\n");
            }

            $sth_feedback = $dbh->prepare("UPDATE images_video SET feedbackactive=:feedbackactive,feedbacklabel=:feedbacklabel,feedbacktime=current_timestamp WHERE session=:session");
            $sth_feedback->execute(array(':feedbackactive'=>0, ':feedbacklabel'=>$feedbacklabel, ':session'=>$session));

            // print(json_encode($within_array));
            // print(json_encode($offset_array));
            
            $current_date = date('d/m/Y==H:i:s');
            print($current_date);
            print($feedbacklabel);
            
        } else {
            print("waiting");
        }
    }
    else {
      print("FAILING");
    }
?>