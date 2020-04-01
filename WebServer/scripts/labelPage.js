// VARS
var img = new Image();
var imgW;
var imgH;

var imageName = null;
var imagePath = null;
var question = null;
var ansType = null;

var imgResponse = null;
var qResponse = null;

var flagVal = false;

session = null;
stateid = null;

// Dynamically set the question being asked
function addResponseHooks() {
	// Reveal the submit button
	$('#legion-submit').show();
	$('#legion-submit').removeAttr("disabled");
	$('#legion-submit').off('click');

	// Handle submission
	$('#legion-submit').click( function() {
		var alllabeled = document.getElementById("alllabeled").checked;
		var notalllabeled = document.getElementById("notalllabeled").checked;
		//detect if alllabeled yes, then different ajax config.

		var flagData = null;

		// TODO: Check if a bound is set
		var isAnswered = false;
		if( x1 != null ) {  // WLOG, pick one coord to check
			isAnswered = true;
		}

		//check size and ratio of bounding box
		var passCheck = true;
		var feedback = "";

		boxWidth = x2 - x1;
		boxHeight = y2 - y1;

		imgWidth = document.getElementById("imageToCrop").naturalWidth;
		imgHeight = document.getElementById("imageToCrop").naturalHeight;

		// if (boxWidth/boxHeight > 10 || boxWidth/boxHeight < 0.1 || boxWidth * boxHeight / (imgWidth * imgHeight) < 0.005) {
		// 	passCheck = false;
		// 	feedback = "What you dragged doesn't seem like a button. Please try again. Thanks!";
		// }

		if ($('#labelText').val() == ""){
			passCheck = false;
			feedback = "You didn't put in any information for the button. Please try again. Thanks!";
		}

		// If there is an answer...
		// if( isAnswered && passCheck) {  // TODO: Is this needed given the returns above?
		// 	// Log the response
		// 	console.log("Writing answer for session " + session + " state " + stateid);
			
		// 	$.ajax({
		// 		url: "php/logLabel.php",
		// 		type: "POST",
		// 		data: {  workerid:gup('workerId'), session: session, stateid: stateid, x1: x1, y1: y1, x2: x2, y2: y2, image: imageName, label: $('#labelText').val().toLowerCase() },
		// 		dataType: "text",
		// 		success: function(d) {
		// 			// Submit the HIT
		// 			alert("Answer submitted. Thanks!");
		// 			submitToTurk(undefined, {"useAlert": false});
		// 		}
		// 	});
		// }
		// else if (isAnswered && !passCheck) {
		// 	alert(feedback);
		// }
		// else 
		if (true) {
			console.log("Writing answer for session " + session + " state " + stateid);
			
			$.ajax({
				url: "php/logLabel.php",
				type: "POST",
				data: {  workerid:gup('workerId'), session: session, stateid: stateid, alllabeled: 'alllabeled'},
				dataType: "text",
				success: function(d) {
					// Submit the HIT
					alert("Answer submitted. Thanks!");
					submitToTurk(undefined, {"useAlert": false});
				}
			});
		}
	});
};

$(document).ready( function() {
	addResponseHooks();
	// $('#legion-submit').hide();
	$('#interface-name').html(gup('session'));

	$('#alllabeled').click( function() {
		if(this.checked) {
			addResponseHooks();
			$('#label-wrapper').hide();
		}
	});

	$('#notalllabeled').click( function() {
		if(this.checked) {
			// $('#legion-submit').hide();
			$('#label-wrapper').show();
		}
	});

	// Let everyone know the image is coming
	console.log("Loading image...");
	console.log("session: " + gup('session'));
	console.log("stateid: " + gup('stateid'));
	$('#image_wrapper').html('<div style="color:#f83; font-size: 24px"><center><b><i>Loading image...</i></b></center></div>');

	// $('#appliancename').html('<div align="center" id="appliancename">Interface Name: <b>' + gup('session') + '</b></div>');

    if( gup("assignmentId") != "ASSIGNMENT_ID_NOT_AVAILABLE" ) {
		// Get the image + question information
		$.ajax({
            url: "php/getReferenceImage.php",
            type: "POST",
            data: { workerid: gup('workerId'), session: gup('session'), stateid: gup('stateid'), task: 'label'},
            dataType: "json",
            success: function(jsonResp) {
				console.log("JSON::", jsonResp)

				active = jsonResp['active'];

				if (active == 1) {
					// TODO: load image
					imageName= jsonResp['image'];
					imagePath = "images_reference/" + gup('session') + "/" + imageName;

					session = jsonResp['session'];
					stateid = jsonResp['stateid'];

					startPosition = 0;
					if (jsonResp['startposition'] != null) {
						startPosition = jsonResp['startposition'];
					};

					console.log("Start position: " + startPosition);

					alllabeled = jsonResp['alllabeled'];
					console.log("All Labeled: " + alllabeled);
					//place arrow in location

					// Set an on-error reload to address 404 errors in images
					img.onerror = function() {
						//location.reload();
					};
					// Display the image
					console.log("Setting image to: ", imageName);
					img.src = imagePath;
					//location.load();
				}
				else {
					// do nothing
				}

				if (alllabeled == 0) {
		   			$('#all-labeled').css("display", "none");
		   		}
		   		else if (alllabeled == 1) {
		   			$('#all-labeled').css("display", "inline");
		   		}

		   		if (active == 0) {
		   			$('#active').css("display", "inline");
		   			$('#all-labeled').css("display", "none");
		   			$('#label-wrapper').css("display", "none");
		   			console.log("Not active any more!!!");
		   		}
		   		else if (active == 1) {
		   			$('#active').css("display", "none");
		   		}
			}
		});

		// Figure out the image size
       	img.onload = function() {
       		//load arrow position
       		if (startPosition == 0) {
       			$('#arrow_wrapper_top').css("float", "left");
				$('#arrow_wrapper_top').html('<img src="css/arrow-down.png"></img>');
				$('#container').css("margin-top", "60px");
       		}
       		else if (startPosition == 1) {
       			$('#arrow_wrapper_bottom').css("float", "left");
				$('#arrow_wrapper_bottom').html('<img src="css/arrow-up.png"></img>');
       		}

			//alert("Loading...");
           	imgW = this.width;
           	imgH = this.height;

			// Load the image as a background and 
			$('#image_wrapper').css("width", imgW);
			$('#image_wrapper').css("height", imgH);

			// Set the image as a background
			$('#image_wrapper').html('<img id="imageToCrop" src="' + this.src + '"></img>');
			//$('#image_wrapper').css("background-image", "url(" + this.src + ")");

			// Once the image is loaded, then we can let people respond
			//addResponseHooks();

			console.log(document.getElementById("image_wrapper").offsetWidth);
			console.log(document.getElementById("imageToCrop").naturalWidth);

			// Add region-selection functions
			initJcrop();
			addOverlays();
		}
    }
    else {
 		$('#image_wrapper').html("<div style='text-align: center;'>Please accept the HIT to continue.</div>");
    }
});

