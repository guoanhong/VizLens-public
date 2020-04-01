<?php session_start(); ?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
	<head>
		<title>Determine image quality and control panel of an appliance</title>
		<!-- Libraries -->
		<script src="https://ajax.googleapis.com/ajax/libs/jquery/1.7.2/jquery.min.js" type="text/javascript"></script>
		<!--script type="text/javascript" src="https://ajax.googleapis.com/ajax/libs/swfobject/2.2/swfobject.js"></script-->
		<script src="scripts/gup.js" type="text/javascript"></script>
		<link rel="stylesheet" type="text/css" href="css/turk.css"></link>
		<link rel="stylesheet" href="css/legion.css" />
		<script src="scripts/vars.js"></script>
		<script src="scripts/legion.js"></script>

		<!-- Scripts -->
		<script src="scripts/cropPage.js" type="text/javascript"></script>

		<!-- Style -->
		<link rel="stylesheet" type="text/css" href="css/style.css"></link>
		<link rel="stylesheet" type="text/css" href="css/overlay.css"></link>

		<!-- JCrop -->
		<script src="jcrop/js/jquery.Jcrop.min.js" type="text/javascript"></script>
		<script src="scripts/region.js" type="text/javascript"></script>

		<!-- Overlay -->
		<script src="scripts/overlay.js" type="text/javascript"></script>
	</head>
	<body>
		<div id="main-interface">
		    <div id="outer-container-crop">
				<div id="header">
					<div id="title">
						Determine Image Quality and Control Panel of an Appliance
					</div>
					<br>
					<div align="center" id="appliancename">
						Appliance Name: <p id="interface-name" style="display:inline">Appliance</p>
					</div>
					<div id="question">
						<p><strong>Note: the image might be large, please scroll around to see the whole image.</strong></p>
						<p>Is the control panel of the appliance COMPLETE? 
						<input id="complete" type="radio" name="complete" value="yes"> Complete  
						<input id="incomplete" type="radio" name="complete" value="no"> Not Complete </p>

						<p>Is the text on the control panel CLEAR to be read? 
						<input id="clear" type="radio" name="clear" value="yes"> Clear   
						<input id="unclear" type="radio" name="clear" value="no"> Not Clear </p>

						<p><b>Note: if the control panel is not both COMPLETE and CLEAR, drag a random area and submit.</b></p>

						<p>Click and drag to select the control panel of the appliance. Please make the bounding box <strong>FIT</strong> the size of the control panel. Please cover <strong>ALL</strong> the buttons, but try NOT to include any background.</p>
					</div>
				</div>

				<div id="container">
					<div id="instr-area"></div>
					<br/>
					<div id="entry_header"></div>
					<div id="image_wrapper"></div>
				</div>

				<div id="submit-div">
					<form id="submit-form">
			  			<input type="hidden" name="money" id="money_field">
						<input type="hidden" name="assignmentId" id="submit-assignmentId">
					</form>
				</div>
		    </div>

		    <!-- Button Count box  -->
		    <div id="label-wrapper">
			    <p><b>Note: if the control panel is not both COMPLETE and CLEAR, skip the counting and description steps.</b></p>
				<p>Please count the number of buttons in the area you selected: <input id="buttons" type="number" style="display:inline;" min="0" value="0"></p>
				
				<p>Please describe the function of the interface:</p>
				<textarea type="textbox" id="description" spellcheck="true"></textarea>
		    </div>
		    <br>
		    <div id="title">Remember to click "Submit HIT" button when you are done.</div>
		</div>

		<div id="sidebar"></div>
		<div id="instructions"></div>
		<p id='legion-money' style="display:none;">.15</p>
	</body>
</html>