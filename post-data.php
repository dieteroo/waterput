<?php

$servername = "localhost";
// REPLACE with your Database name
$dbname = "esp_data";
// REPLACE with Database user
$username = "Database_user";
// REPLACE with Database user password
$password = "Database_user_password";

// Keep this API Key value to be compatible with the ESP32 code provided in the project page. 
// If you change this value, the ESP32 sketch needs to match.
$api_key_value = "ZetHierIetsWatUniekIs";

// Clear values just to be sure.
$api_key = $diepte = $volume = "";

if ($_SERVER["REQUEST_METHOD"] == "POST") {
    $api_key = test_input($_POST["api_key"]);
    if($api_key == $api_key_value) {
        $diepte = test_input($_POST["diepte"]);
        $volume = test_input($_POST["volume"]);

        // Create connection
        $conn = new mysqli($servername, $username, $password, $dbname);
        // Check connection
        if ($conn->connect_error) {
            die("Connection failed: " . $conn->connect_error);
        }

        $sql = "INSERT INTO Waterput (diepte, volume)
        VALUES ('" . $diepte . "', '" . $volume . "')";

        if ($conn->query($sql) === TRUE) {
            echo "New record created successfully";
        }
        else {
            echo "Error: " . $sql . "<br>" . $conn->error;
        }

        $conn->close();
    }
    else {
        echo "Wrong API Key provided.";
    }

}
else {
    echo "No data posted with HTTP POST.";
}

function test_input($data) {
    $data = trim($data);
    $data = stripslashes($data);
    $data = htmlspecialchars($data);
    return $data;
}
