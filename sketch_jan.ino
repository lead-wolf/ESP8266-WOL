#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 
#include <WiFiUdp.h>
#include <FS.h>

#define MAX_SESSIONS 5
ESP8266WebServer server(21001);
WiFiUDP udp;
WiFiManager wm;

struct SessionLogin {
  IPAddress  loggedInIP;
  unsigned long lastActive;
};

struct Device {
  String name;
  String mac;
  String ip;
  String broadcast;
  int port;
};

struct User {
  String user_login;
  String pass_login;
};

struct Pin {
  String pin_wol;
  bool status;
};

struct ForwardedNote {
  String name;
  String internal_port;
  String forwarded_port;
  String ip_nat;
  String description;
};

unsigned long lastResetTime = 0;
const String DEFAULT_USER = "admin";
const String DEFAULT_PASS = "admin123";
const String DEFAULT_PIN = "123456";
SessionLogin sessions[MAX_SESSIONS];
std::vector<Device> devices;
std::vector<User> users;
std::vector<Pin> pins;
std::vector<ForwardedNote> forwarded_notes;

const char login_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
      <head>
        <meta charset="UTF-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1.0" />
        <title>Login - AN HOME NETWORK</title>
        <style>
          :root {
            --prim: #53e3a6;
          }
          * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;

            font-weight: 300;
          }

          body {
            font-family: "Source Sans Pro", sans-serif;
            color: white;
            font-weight: 300;

            ::-webkit-input-placeholder {
              /* WebKit browsers */
              font-family: "Source Sans Pro", sans-serif;
              color: white;
              font-weight: 300;
            }
            :-moz-placeholder {
              /* Mozilla Firefox 4 to 18 */
              font-family: "Source Sans Pro", sans-serif;
              color: white;
              opacity: 1;
              font-weight: 300;
            }
            ::-moz-placeholder {
              /* Mozilla Firefox 19+ */
              font-family: "Source Sans Pro", sans-serif;
              color: white;
              opacity: 1;
              font-weight: 300;
            }
            :-ms-input-placeholder {
              /* Internet Explorer 10+ */
              font-family: "Source Sans Pro", sans-serif;
              color: white;
              font-weight: 300;
            }
          }

          .wrapper {
            background: #50a3a2;
            background: -webkit-linear-gradient(top left, #50a3a2 0%, #53e3a6 100%);
            background: -moz-linear-gradient(top left, #50a3a2 0%, #53e3a6 100%);
            background: -o-linear-gradient(top left, #50a3a2 0%, #53e3a6 100%);
            background: linear-gradient(to bottom right, #50a3a2 0%, #53e3a6 100%);

            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            margin-top: 0px;
            overflow: hidden;
          }

          .wrapper.form-success .container h1 {
            transform: translateY(85px);
          }

          .container {
            max-width: 600px;
            margin-top: 50px !important;
            margin: 0 auto;
            padding: 80px 0;
            height: 400px;
            text-align: center;
          }

          .container h1 {
            font-size: 40px;
            transition-duration: 1s;
            transition-timing-function: ease-in-out;
            font-weight: 200;
          }

          form {
            padding: 20px 0;
            position: relative;
            z-index: 2;
          }

          form input {
            display: block;
            outline: 0;
            border: 1px solid rgba(255, 255, 255, 0.4);
            background-color: rgba(255, 255, 255, 0.2);
            width: 250px;
            border-radius: 3px;
            padding: 10px 15px;
            margin: 0 auto 10px auto;
            text-align: center;
            font-size: 18px;
            color: white;
            transition-duration: 0.25s;
            font-weight: 300;
          }

          form input:hover {
            background-color: rgba(255, 255, 255, 0.4);
          }

          form input:focus {
            background-color: white;
            width: 300px;
            color: var(--prim);
          }

          form button {
            background-color: white;
            border: 0;
            padding: 10px 15px;
            color: var(--prim);
            border-radius: 3px;
            width: 250px;
            cursor: pointer;
            font-size: 18px;
            transition-duration: 0.25s;
          }

          form button:hover {
            background-color: rgb(245, 247, 249);
          }

          .bg-bubbles {
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;

            z-index: 1;
          }

          .bg-bubbles li {
            position: absolute;
            list-style: none;
            display: block;
            width: 40px;
            height: 40px;
            background-color: rgba(255, 255, 255, 0.15);
            bottom: -160px;
            animation: square 25s infinite linear;
          }

          .bg-bubbles li:nth-child(1) {
            left: 10%;
          }

          .bg-bubbles li:nth-child(2) {
            left: 20%;

            width: 80px;
            height: 80px;

            animation-delay: 2s;
            animation-duration: 17s;
          }

          .bg-bubbles li:nth-child(3) {
            left: 25%;
            animation-delay: 4s;
          }

          .bg-bubbles li:nth-child(4) {
            left: 40%;
            width: 60px;
            height: 60px;

            animation-duration: 22s;

            background-color: fade(white, 25%);
          }

          .bg-bubbles li:nth-child(5) {
            left: 70%;
          }

          .bg-bubbles li:nth-child(6) {
            left: 80%;
            width: 120px;
            height: 120px;

            animation-delay: 3s;
            background-color: fade(white, 20%);
          }

          .bg-bubbles li:nth-child(7) {
            left: 32%;
            width: 160px;
            height: 160px;

            animation-delay: 7s;
          }

          .bg-bubbles li:nth-child(8) {
            left: 55%;
            width: 20px;
            height: 20px;

            animation-delay: 15s;
            animation-duration: 40s;
          }

          .bg-bubbles li:nth-child(9) {
            left: 25%;
            width: 10px;
            height: 10px;

            animation-delay: 2s;
            animation-duration: 40s;
            background-color: fade(white, 30%);
          }

          .bg-bubbles li:nth-child(10) {
            left: 90%;
            width: 160px;
            height: 160px;

            animation-delay: 11s;
          }

          @-webkit-keyframes square {
            0% {
              transform: translateY(0);
            }
            100% {
              transform: translateY(-700px) rotate(600deg);
            }
          }
          @keyframes square {
            0% {
              transform: translateY(0);
            }
            100% {
              transform: translateY(-700px) rotate(600deg);
            }
          }

          #valid-input {
            margin-top: 10px;
            color: #db3232;
            display: none;
          }

          #login-fail {
            margin-top: 10px;
            color: #db3232;
            display: none;
          }

          .spinner {
            border: 3px solid #f3f3f3;
            border-top: 3px solid #3498db;
            border-radius: 50%;
            width: 16px;
            height: 16px;
            animation: spin 1s linear infinite;
            display: inline-block;
            vertical-align: middle;
            margin-left: 5px;
          }

          @keyframes spin {
            0% {
              transform: rotate(0deg);
            }
            100% {
              transform: rotate(360deg);
            }
          }
        </style>
      </head>
      <body class="dark">
        <div class="wrapper">
          <div class="container">
            <h1>Welcome</h1>

            <h3 id="login-fail">
              Tài khoản hoặc mật khẩu không chính xác <br />Vui lòng thử lại
            </h3>
            <h3 id="valid-input">Username và Password không được để trống</h3>

            <form class="form">
              <input id="username" type="text" placeholder="Username" />
              <input id="password" type="password" placeholder="Password" />
              <button type="submit" id="login-button">Login</button>
            </form>
          </div>

          <ul class="bg-bubbles">
            <li></li>
            <li></li>
            <li></li>
            <li></li>
            <li></li>
            <li></li>
            <li></li>
            <li></li>
            <li></li>
            <li></li>
          </ul>
        </div>
        <script src="https://cdnjs.cloudflare.com/ajax/libs/jquery/3.7.1/jquery.min.js"></script>
        <script>
          $("#login-button").click(function (event) {
            event.preventDefault();

            var username = $("#username").val();
            var password = $("#password").val();

            if (!username || username == "" || !password) {
              $("#valid-input").css("display", "block");
              return;
            } else {
              $("#valid-input").css("display", "none");
            }

            $("#login-button").html('Logging in <div class="spinner"></div>');
            $("#valid-input").css("display", "none");
            $("#login-fail").css("display", "none");

            $.ajax({
              url: "/login",
              type: "POST",
              data: {
                username: username,
                password: password,
              },
              success: function (response) {
                console.log(response);
                if (response == "success") {
                  $("form").fadeOut(500);  
                  $(".wrapper").addClass("form-success");
                  setTimeout(() => {
                   window.location.href = "/manage";
                  }, 1500);
                } else {
                  $("#login-fail").css("display", "block");
                  $("#login-button").html("Login");
                  $("#login-button").prop("disabled", false);
                }
              },
              error: function (xhr, status, error) {
                $("#login-fail").css("display", "block");
                $("#login-button").html("Login");
                $("#login-button").prop("disabled", false);
              },
            });
          });
        </script>
      </body>
    </html>
)rawliteral";

const char manage_header_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
    <head>
      <meta charset="UTF-8" />
      <meta name="viewport" content="width=device-width, initial-scale=1.0" />
      <link rel="icon"
      href="https://cdn-icons-png.freepik.com/512/6329/6329326.png"
      type=image/png>
      <title>Admin - AN HOME NETWORK</title>
       <link
      href="https://cdn.boxicons.com/fonts/basic/boxicons.min.css"
      rel="stylesheet"
      />
      <link
        href="https://unpkg.com/boxicons@2.1.4/css/boxicons.min.css"
        rel="stylesheet"
      />
      <script src="https://unpkg.com/boxicons@2.1.4/dist/boxicons.js"></script>
      <script src="https://cdnjs.cloudflare.com/ajax/libs/jquery/3.7.1/jquery.min.js"></script>
      <script src="https://cdn.jsdelivr.net/npm/axios/dist/axios.min.js"></script>
      <script src="https://cdn.jsdelivr.net/npm/sweetalert2@11"></script>
      <style>
        * {
          margin: 0;
          padding: 0;
          box-sizing: border-box;
        }

        a {
          text-decoration: none;
        }

        li {
          list-style: none;
        }

        :root {
          --poppins: "Poppins", sans-serif;
          --lato: "Lato", sans-serif;

          --light: #f9f9f9;
          --blue: #3c91e6;
          --light-blue: #cfe8ff;
          --grey: #eee;
          --dark-grey: #aaaaaa;
          --dark: #342e37;
          --red: #db504a;
          --yellow: #ffce26;
          --light-yellow: #fff2c6;
          --orange: #fd7238;
          --light-orange: #ffe0d3;
        }

        html {
          overflow-x: hidden;
        }

        body.dark {
          --light: #0c0c1e;
          --grey: #060714;
          --dark: #fbfbfb;
        }

        body {
          background: var(--grey);
          overflow-x: hidden;
        }

        /* SIDEBAR */
        #sidebar {
          position: fixed;
          top: 0;
          left: 0;
          width: 220px;
          height: 100%;
          background: var(--light);
          z-index: 2000;
          font-family: var(--lato);
          transition: 0.3s ease;
          overflow-x: hidden;
          scrollbar-width: none;
        }
        #sidebar::--webkit-scrollbar {
          display: none;
        }
        #sidebar.hide {
          width: 60px;
        }
        #sidebar .brand {
          font-size: 24px;
          font-weight: 700;
          height: 56px;
          display: flex;
          align-items: center;
          color: var(--blue);
          position: sticky;
          top: 0;
          left: 0;
          background: var(--light);
          z-index: 500;
          padding-bottom: 20px;
          box-sizing: content-box;
        }
        #sidebar .brand .bx {
          min-width: 60px;
          display: flex;
          justify-content: center;
        }
        #sidebar .side-menu {
          width: 100%;
          margin-top: 48px;
        }
        #sidebar .side-menu li {
          height: 48px;
          background: transparent;
          margin-left: 6px;
          border-radius: 48px 0 0 48px;
          padding: 4px;
        }
        #sidebar .side-menu li.active {
          background: var(--grey);
          position: relative;
        }
        #sidebar .side-menu li.active::before {
          content: "";
          position: absolute;
          width: 40px;
          height: 40px;
          border-radius: 50%;
          top: -40px;
          right: 0;
          box-shadow: 20px 20px 0 var(--grey);
          z-index: -1;
        }
        #sidebar .side-menu li.active::after {
          content: "";
          position: absolute;
          width: 40px;
          height: 40px;
          border-radius: 50%;
          bottom: -40px;
          right: 0;
          box-shadow: 20px -20px 0 var(--grey);
          z-index: -1;
        }
        #sidebar .side-menu li a {
          width: 100%;
          height: 100%;
          background: var(--light);
          display: flex;
          align-items: center;
          border-radius: 48px;
          font-size: 16px;
          color: var(--dark);
          white-space: nowrap;
          overflow-x: hidden;
        }
        #sidebar .side-menu.top li.active a {
          color: var(--blue);
        }
        #sidebar.hide .side-menu li a {
          width: calc(48px - (4px * 2));
          transition: width 0.3s ease;
        }
        #sidebar .side-menu li a.logout {
          color: var(--red);
        }
        #sidebar .side-menu.top li a:hover {
          color: var(--blue);
        }
        #sidebar .side-menu li a .bx {
          min-width: calc(60px - ((4px + 6px) * 2));
          display: flex;
          justify-content: center;
        }

        #sidebar .side-menu.bottom li:nth-last-of-type(-n + 2) {
          margin-bottom: 10px;
          position: absolute;
          bottom: 0;
          left: 0;
          right: 0;
          text-align: center;
        }

        #sidebar .side-menu.bottom li:nth-last-of-type(2) {
          bottom: 40px;
        }
        /* SIDEBAR */

        /* CONTENT */
        #content {
          position: relative;
          width: calc(100% - 220px);
          left: 220px;
          transition: 0.3s ease;
        }
        #sidebar.hide ~ #content {
          width: calc(100% - 60px);
          left: 60px;
        }

        /* NAVBAR */
        #content nav {
          height: 56px;
          background: var(--light);
          padding: 0 24px;
          display: flex;
          align-items: center;
          grid-gap: 24px;
          font-family: var(--lato);
          position: sticky;
          top: 0;
          left: 0;
          z-index: 1000;
        }
        #content nav::before {
          content: "";
          position: absolute;
          width: 40px;
          height: 40px;
          bottom: -40px;
          left: 0;
          border-radius: 50%;
          box-shadow: -20px -20px 0 var(--light);
        }
        #content nav a {
          color: var(--dark);
        }
        #content nav .bx.bx-menu {
          cursor: pointer;
          color: var(--dark);
        }
        #content nav .nav-link {
          font-size: 16px;
          transition: 0.3s ease;
        }
        #content nav .nav-link:hover {
          color: var(--blue);
        }
        #content nav form {
          max-width: 400px;
          width: 100%;
          margin-right: auto;
        }
        #content nav form .form-input {
          display: flex;
          align-items: center;
          height: 36px;
        }
        #content nav form .form-input input {
          flex-grow: 1;
          padding: 0 16px;
          height: 100%;
          border: none;
          background: var(--grey);
          border-radius: 36px 0 0 36px;
          outline: none;
          width: 100%;
          color: var(--dark);
        }
        #content nav form .form-input button {
          width: 36px;
          height: 100%;
          display: flex;
          justify-content: center;
          align-items: center;
          background: var(--blue);
          color: var(--light);
          font-size: 18px;
          border: none;
          outline: none;
          border-radius: 0 36px 36px 0;
          cursor: pointer;
        }

        #content nav .swith-lm {
          background-color: var(--grey);
          border-radius: 50px;
          cursor: pointer;
          display: flex;
          align-items: center;
          justify-content: space-between;
          padding: 3px;
          position: relative;
          height: 21px;
          width: 45px;
          transform: scale(1.5);
        }

        #content nav .swith-lm .ball {
          background-color: var(--blue);
          border-radius: 50%;
          position: absolute;
          top: 2px;
          left: 2px;
          height: 20px;
          width: 20px;
          transform: translateX(0px);
          transition: transform 0.2s linear;
        }

        #content nav .checkbox:checked + .swith-lm .ball {
          transform: translateX(22px);
        }
        .bxs-moon {
          color: var(--yellow);
        }

        .bx-sun {
          color: var(--orange);
          animation: shakeOn 0.7s;
        }

        /* NAVBAR */

        /* MAIN */
        #content main {
          display: none;
          width: 100%;
          padding: 36px 24px;
          font-family: var(--poppins);
          max-height: calc(100vh - 56px);
          overflow-y: auto;
        }

        #content main.show {
          display: block;
        }

        #content main .head-title {
          display: flex;
          align-items: center;
          justify-content: space-between;
          grid-gap: 16px;
          flex-wrap: wrap;
        }
        #content main .head-title .left h1 {
          font-size: 36px;
          font-weight: 600;
          margin-bottom: 10px;
          color: var(--dark);
        }
        #content main .head-title .left .breadcrumb {
          display: flex;
          align-items: center;
          grid-gap: 16px;
        }
        #content main .head-title .left .breadcrumb li {
          color: var(--dark);
        }
        #content main .head-title .left .breadcrumb li a {
          color: var(--dark-grey);
          pointer-events: none;
        }
        #content main .head-title .left .breadcrumb li a.active {
          color: var(--blue);
          pointer-events: unset;
        }
        #content main .head-title .btn-download {
          height: 36px;
          padding: 0 16px;
          border-radius: 36px;
          background: var(--blue);
          color: var(--light);
          display: flex;
          justify-content: center;
          align-items: center;
          grid-gap: 10px;
          font-weight: 500;
        }

        #content main .box-info {
          display: grid;
          grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
          grid-gap: 24px;
          margin-top: 36px;
        }
        #content main .box-info li {
          padding: 24px;
          background: var(--light);
          border-radius: 20px;
          display: flex;
          align-items: center;
          grid-gap: 24px;
        }
        #content main .box-info li .bx {
          width: 80px;
          height: 80px;
          border-radius: 10px;
          font-size: 36px;
          display: flex;
          justify-content: center;
          align-items: center;
        }
        #content main .box-info li:nth-child(1) .bx {
          background: var(--light-blue);
          color: var(--blue);
        }
        #content main .box-info li:nth-child(2) .bx {
          background: var(--light-yellow);
          color: var(--yellow);
        }
        #content main .box-info li:nth-child(3) .bx {
          background: var(--light-orange);
          color: var(--orange);
        }
        #content main .box-info li:nth-child(4) .bx {
          background: var(--light-orange);
          color: var(--orange);
        }
        #content main .box-info li .text h3 {
          font-size: 24px;
          font-weight: 600;
          color: var(--dark);
        }
        #content main .box-info li .text p {
          color: var(--dark);
        }

        #content main .table-data {
          display: flex;
          flex-wrap: wrap;
          grid-gap: 24px;
          margin-top: 24px;
          width: 100%;
          color: var(--dark);
        }
        #content main .table-data > div {
          border-radius: 20px;
          background: var(--light);
          padding: 24px;
          overflow-x: auto;
        }
        #content main .table-data .head {
          display: flex;
          align-items: center;
          grid-gap: 16px;
          margin-bottom: 24px;
        }
        #content main .table-data .head h3 {
          margin-right: auto;
          font-size: 24px;
          font-weight: 600;
        }
        #content main .table-data .head .bx {
          cursor: pointer;
        }

        #content main .table-data .order {
          flex-grow: 1;
          flex-basis: 500px;
        }
        #content main .table-data .order table {
          width: 100%;
          border-collapse: collapse;
          text-align-last: center;
        }
        #content main .table-data .order table th {
          padding-bottom: 12px;
          font-size: 13px;
          text-align: left;
          border-bottom: 1px solid var(--grey);
        }
        #content main .table-data .order table td {
          padding: 16px 0;
        }
        #content main .table-data .order table tr td:first-child {
          /* display: flex; */
          align-items: center;
          grid-gap: 12px;
          padding-left: 6px;
        }
        #content main .table-data .order table td img {
          width: 36px;
          height: 36px;
          border-radius: 50%;
          object-fit: cover;
        }
        #content main .table-data .order table tbody tr:hover {
          background: var(--grey);
        }
        #content main .table-data .order table tr td .status {
          font-size: 10px;
          padding: 6px 16px;
          color: var(--light);
          border-radius: 20px;
          font-weight: 700;
        }
        #content main .table-data .order table tr td .status.completed {
          background: var(--blue);
        }
        #content main .table-data .order table tr td .status.process {
          background: var(--yellow);
        }
        #content main .table-data .order table tr td .status.pending {
          background: var(--orange);
        }

        #content main .table-data .todo {
          flex-grow: 1;
          flex-basis: 300px;
        }
        #content main .table-data .todo .todo-list {
          width: 100%;
        }
        #content main .table-data .todo .todo-list li {
          width: 100%;
          margin-bottom: 16px;
          background: var(--grey);
          border-radius: 10px;
          padding: 14px 20px;
          display: flex;
          justify-content: space-between;
          align-items: center;
        }
        #content main .table-data .todo .todo-list li .bx {
          cursor: pointer;
        }
        #content main .table-data .todo .todo-list li.completed {
          border-left: 10px solid var(--blue);
        }
        #content main .table-data .todo .todo-list li.not-completed {
          border-left: 10px solid var(--orange);
        }
        #content main .table-data .todo .todo-list li:last-child {
          margin-bottom: 0;
        }
        /* MAIN */
        /* CONTENT */
        #content main .menu,
        #content nav .menu {
          display: none;
          list-style-type: none;
          padding-left: 20px;
          margin-top: 5px;
          position: absolute;
          background-color: #f9f9f9;
          border: 1px solid #ddd;
          border-radius: 5px;
          width: 200px;
        }

        #content main .menu a,
        #content nav .menu a {
          color: white;
          text-decoration: none;
          display: block;
          padding: 8px 16px;
        }

        #content main .menu a:hover,
        #content nav .menu a:hover {
          background-color: #444;
        }

        #content main .menu-link,
        #content nav .menu-link {
          margin: 5px;
          padding: 10px 20px;
          font-size: 16px;
          cursor: pointer;
          text-decoration: none;
          color: #007bff;
        }

        #content main .menu-link:hover,
        #content nav .menu-link:hover {
          text-decoration: underline;
        }

        /* Media Query for Smaller Screens */
        @media screen and (max-width: 768px) {
          /* Reduce width of notification and profile menu */
          #content nav .notification-menu,
          #content nav .profile-menu {
            width: 180px;
          }
          #sidebar {
            width: 200px;
          }

          #content {
            width: calc(100% - 60px);
            left: 200px;
          }

          #content nav .nav-link {
            display: none;
          }
        }

        @media screen and (max-width: 576px) {
          #content nav .notification-menu,
          #content nav .profile-menu {
            width: 150px;
          }
          #content nav form .form-input input {
            display: none;
          }

          #content nav form .form-input button {
            width: auto;
            height: auto;
            background: transparent;
            border-radius: none;
            color: var(--dark);
          }

          #content nav form.show .form-input input {
            display: block;
            width: 100%;
          }
          #content nav form.show .form-input button {
            width: 36px;
            height: 100%;
            border-radius: 0 36px 36px 0;
            color: var(--light);
            background: var(--red);
          }

          #content nav form.show ~ .notification,
          #content nav form.show ~ .profile {
            display: none;
          }

          #content main .box-info {
            grid-template-columns: 1fr;
          }

          #content main .table-data .head {
            min-width: 420px;
          }
          #content main .table-data .order table {
            min-width: 420px;
          }
          #content main .table-data .todo .todo-list {
            min-width: 420px;
          }
        }
        .rwd-table {
          margin: 1em 0;
          min-width: 300px;
        }

        .rwd-table tr {
          border-top: 1px solid #ddd;
          border-bottom: 1px solid #ddd;
        }

        .rwd-table th {
          display: none;
        }

        .rwd-table td {
          display: block;
        }

        .rwd-table td:first-child {
          padding-top: 0.5em;
        }

        .rwd-table td:last-child {
          padding-bottom: 0.5em;
        }

        .rwd-table td:before {
          content: attr(data-th) ": ";
          font-weight: bold;
          width: 6.5em;
          display: inline-block;
        }

        @media (min-width: 480px) {
          .rwd-table td:before {
            display: none;
          }
        }

        .rwd-table th,
        .rwd-table td {
          text-align: left;
        }

        @media (min-width: 480px) {
          .rwd-table th,
          .rwd-table td {
            display: table-cell;
            padding: 0.25em 0.5em;
          }
          .rwd-table th:first-child,
          .rwd-table td:first-child {
            padding-left: 0;
          }
          .rwd-table th:last-child,
          .rwd-table td:last-child {
            padding-right: 0;
          }
        }

        /* Presentational styling */

        h1 {
          font-weight: normal;
          letter-spacing: -1px;
          color: #34495e;
        }

        .rwd-table {
          background: #34495e;
          color: #fff;
          border-radius: 0.4em;
          overflow: hidden;
        }

        .rwd-table tr {
          border-color: #415b76; /* lighten(#34495E, 10%) */
        }

        .rwd-table th,
        .rwd-table td {
          margin: 0.5em 1em;
        }

        @media (min-width: 480px) {
          .rwd-table th,
          .rwd-table td {
            padding: 1em !important;
          }
        }

        .rwd-table th,
        .rwd-table td:before {
          color: #dd5;
        }
      </style>
      <style>
        .modal-add-device {
          display: none;
          position: fixed;
          z-index: 9999;
          left: 0;
          top: 0;
          width: 100vw;
          height: 100vh;
          background: rgba(0, 0, 0, 0.35);
          justify-content: center;
          align-items: center;
        }
        .modal-add-device.show {
          display: flex;
        }
        .modal-content-add {
          background: #fff;
          border-radius: 16px;
          padding: 32px 28px 24px 28px;
          width: 30%;
          min-width: 320px;
          max-width: 95vw;
          box-shadow: 0 8px 32px rgba(60, 145, 230, 0.18);
          position: relative;
          animation: modalFadeIn 0.2s;
        }
        @keyframes modalFadeIn {
          from {
            transform: translateY(-40px);
            opacity: 0;
          }
          to {
            transform: translateY(0);
            opacity: 1;
          }
        }
        .close-add-device {
          position: absolute;
          top: 12px;
          right: 18px;
          font-size: 28px;
          color: #aaa;
          cursor: pointer;
          transition: color 0.2s;
        }
        .close-add-device:hover {
          color: #3c91e6;
        }
        .add-device-form {
          display: flex;
          flex-direction: column;
          gap: 10px;
        }
        .add-device-form label {
          font-weight: 500;
          color: #3c91e6;
          margin-bottom: 2px;
          margin-top: 8px;
        }
        .add-device-form input[type="text"],
        .add-device-form input[type="password"],
        .add-device-form select,
        .add-device-form input[type="number"] {
          padding: 8px 12px;
          border: 1px solid #cfe8ff;
          border-radius: 6px;
          font-size: 15px;
          outline: none;
          transition: border 0.2s;
        }
        .add-device-form input:focus {
          border: 1.5px solid #3c91e6;
          background: #f5faff;
        }
        .add-device-form input[type="submit"] {
          background: #3c91e6;
          color: #fff;
          border: none;
          border-radius: 8px;
          padding: 10px 0;
          font-size: 16px;
          font-weight: 600;
          cursor: pointer;
          transition: background 0.2s;
        }
        .add-device-form input[type="submit"]:hover {
          background: #2566a6;
        }
      </style>
    </head>
    <body>
)rawliteral";

const char manage_slidebar_html[] PROGMEM = R"rawliteral(
  <section id="sidebar">
      <a href="/manage" class="brand">
        <i class="bx bxs-smile bx-lg"></i>
        <span class="text">AdminHub</span>
      </a>
      <ul class="side-menu top">
        <li class="" data-tab="dashboard">
          <a href="#">
            <i class="bx bxs-dashboard bx-sm"></i>
            <span class="text">Dashboard</span>
          </a>
        </li>
        <li data-tab="devices">
          <a href="#">
            <i class="bx bxs-devices bx-sm"></i>
            <span class="text">My Device</span>
          </a>
        </li>
        <li data-tab="users">
          <a href="#">
            <i class="bx bx-user"></i>
            <span class="text">User Management</span>
          </a>
        </li>
        <li data-tab="pins">
          <a href="#">
            <i class="bx bx-key"></i>
            <span class="text">PIN Management</span>
          </a>
        </li>
      </ul>
      <ul class="side-menu bottom">
        <li>
          <a href="/logout" class="logout">
            <i class="bx bx-power-off bx-sm bx-burst-hover"></i>
            <span class="text">Logout</span>
          </a>
        </li>
      </ul>
    </section>

    <section id="content">
)rawliteral";

const char manage_navbar_html[] PROGMEM = R"rawliteral(
      <nav>
        <i class="bx bx-menu bx-sm"></i>
        <input
          type="checkbox"
          class="checkbox"
          id="switch-mode"
          hidden
          checked="true"
        />
        <label class="swith-lm" for="switch-mode">
          <i class="bx bxs-moon"></i>
          <i class="bx bx-sun"></i>
          <div class="ball"></div>
        </label>
      </nav>
)rawliteral";

const char manage_dashboad_html_0[] PROGMEM = R"rawliteral(
      <main id="mainContent">
        <div class="head-title">
          <div class="left">
            <h1>Dashboard</h1>
            <ul class="breadcrumb">
              <li>
                <a href="#">Dashboard</a>
              </li>
              <li><i class="bx bx-chevron-right"></i></li>
              <li>
                <a class="active" href="#">Home</a>
              </li>
            </ul>
          </div>
        </div>

        <ul class="box-info">
          <li>
            <i class="bx bxs-devices bx-sm"></i>
            <span class="text">
)rawliteral";

const char manage_dashboad_html_1[] PROGMEM = R"rawliteral(
              <p>Devices</p>
            </span>
          </li>
          <li>
            <i class="bx bxs-group"></i>
            <span class="text">
)rawliteral";

const char manage_dashboad_html_2[] PROGMEM = R"rawliteral(
              <p>User</p>
            </span>
          </li>
          <li style="justify-content: center">
            <a
              href="/esp-restart"
              style="justify-items: center"
              class="confirm-restart"
            >
              <i
                class="bx bx-refresh"
                style="color: #e67c4b; font-size: xxx-large"
              ></i>
              <span class="text">
                <h3 style="color: chartreuse; margin-top: 15px">Restart ESP</h3>
              </span>
            </a>
          </li>
          <li style="justify-content: center">
            <a
              href="/hard-reset"
              style="justify-items: center"
              class="confirm-reset"
            >
              <i
                class="bx bx-compare-alt"
                style="color: #e61212; font-size: xxx-large"
              ></i>
              <span class="text">
                <h3 style="color: crimson; margin-top: 15px">
                  Reset network setting
                </h3>
              </span>
            </a>
          </li>
        </ul>

        <div class="table-data">
          <div class="order">
            <div class="head">
              <h3>Ports Forwarded note</h3>
              <i class="bx bx-plus" id="btnAddPortNote"></i>
            </div>
            <table>
              <thead>
                <tr>
                  <th>STT</th>
                  <th>Name</th>
                  <th>Internal Port</th>
                  <th>Forrwarded Port</th>
                  <th>IP NAT</th>
                  <th>Description</th>
                  <th>Delete</th>
                </tr>
              </thead>
              <tbody>
)rawliteral";

const char manage_dashboad_html_3[] PROGMEM = R"rawliteral(
              </tbody>
            </table>
          </div>
        </div>
        <div id="addPortNoteModal" class="modal-add-device">
          <div class="modal-content-add">
            <span class="close-add-device" id="closeAddPortNote">&times;</span>
            <h2 style="margin-bottom: 18px">Thêm Ports Forwarded note</h2>
            <h4 id="addPortNoteError" style="color: crimson"></h4>
            <form
              method="POST"
              action="/add-forwarded-note"
              class="add-device-form"
              id="formAddPortNote"
            >
              <label>Tên</label>
              <input
                name="name"
                type="text"
                required
                placeholder="VD: Server SSH"
              />
              <label>Internal Port</label>
              <div style="display: flex; gap: 8px">
                <input
                  name="internal_port_start"
                  type="number"
                  min="1"
                  max="65535"
                  required
                  placeholder="Start"
                  style="flex: 1"
                />
                <input
                  name="internal_port_end"
                  type="number"
                  min="1"
                  max="65535"
                  required
                  placeholder="End"
                  style="flex: 1"
                />
              </div>
              <label>Forwarded Port</label>
              <div style="display: flex; gap: 8px">
                <input
                  name="forwarded_port_start"
                  type="number"
                  min="1"
                  max="65535"
                  required
                  placeholder="Start"
                  style="flex: 1"
                />
                <input
                  name="forwarded_port_end"
                  type="number"
                  min="1"
                  max="65535"
                  required
                  placeholder="End"
                  style="flex: 1"
                />
              </div>
              <label>IP NAT</label>
              <input
                name="ip_nat"
                type="text"
                required
                placeholder="10.10.10.252"
              />
              <label>Description</label>
              <input
                name="description"
                type="text"
                placeholder="Nat for SSH server"
              />
              <input type="submit" value="Thêm" style="margin-top: 10px" />
            </form>
          </div>
        </div>
      </main>
)rawliteral";

const char manage_device_html_start[] PROGMEM = R"rawliteral(
     <main id="devices-content">
        <h1 style="justify-self: center">Device Management</h1>
        <table class="rwd-table" style="justify-self: center">
          <tr>
            <th>Device Name</th>
            <th>Device Mac</th>
            <th>Device IP</th>
            <th>Device Port WOL</th>
            <th>Device Broadcast Address</th>
            <th>Action</th>
          </tr>
)rawliteral";

const char manage_device_html_end[] PROGMEM = R"rawliteral(
          <tr>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td data-th="Action">
              <button
                id="btnAddDevice"
                style="
                  margin: 20px 0 10px 0;
                  padding: 10px 24px;
                  background: #3c91e6;
                  color: #fff;
                  border: none;
                  border-radius: 8px;
                  font-size: 16px;
                  cursor: pointer;
                  transition: background 0.2s;
                "
              >
                + Thêm thiết bị
              </button>
            </td>
          </tr>
        </table>

        <div id="addDeviceModal" class="modal-add-device">
          <div class="modal-content-add">
            <span class="close-add-device" id="closeAddDevice">&times;</span>
            <h2 style="margin-bottom: 18px">Thêm thiết bị</h2>
            <form method="POST" action="/add-device" class="add-device-form">
              <label>Tên thiết bị</label>
              <input
                name="name"
                type="text"
                required
                placeholder="VD: PC Văn phòng"
              />
              <label>MAC</label>
              <input
                name="mac"
                type="text"
                required
                placeholder="12:34:56:78:9a:0b"
              />
              <label>IP</label>
              <input name="ip" type="text" required placeholder="10.10.10.10" />
              <label>Broadcast</label>
              <input
                name="broadcast"
                type="text"
                required
                placeholder="255.255.255.255"
              />
              <label>Port</label>
              <input
                name="port"
                required
                placeholder="40000"
                type="number"
                min="1"
                max="65535"
              />
              <input
                type="submit"
                value="Thêm"
                style="margin-top: 10px"
              />
            </form>
          </div>
        </div>
      </main>
)rawliteral";

const char manage_user_html_start[] PROGMEM = R"rawliteral(
      <main id="users-content" style="display: none">
        <h1 style="justify-self: center">User Management</h1>
        <table
          class="rwd-table"
          style="justify-self: center; min-width: 50%; text-align-last: center"
        >
          <tr>
            <th>Username</th>
            <th>Password</th>
            <th>Change Password</th>
            <th style="color: crimson">Delete</th>
          </tr>
)rawliteral";

const char manage_user_html_end[] PROGMEM = R"rawliteral(
          <tr>
            <td></td>
            <td></td>
            <td></td>
            <td>
              <h4 id="noti"></h4>
              <button
                id="btnAddUser"
                style="
                  margin: 20px 0 10px 0;
                  padding: 10px 24px;
                  background: #3c91e6;
                  color: #fff;
                  border: none;
                  border-radius: 8px;
                  font-size: 16px;
                  cursor: pointer;
                  transition: background 0.2s;
                "
              >
                + Thêm user
              </button>
            </td>
          </tr>
        </table>

        <!-- Modal Thêm User -->
        <div id="addUserModal" class="modal-add-device">
          <div class="modal-content-add">
            <span class="close-add-device" id="closeAddUser">&times;</span>
            <h2 style="margin-bottom: 18px">Thêm user</h2>
            <form method="POST" action="/add-user" class="add-device-form">
              <label>Tên đăng nhập</label>
              <input
                name="username"
                minlength="3"
                type="text"
                pattern="\S+"
                required
                placeholder="Tên đăng nhập"
              />
              <label>Mật khẩu</label>
              <input
                name="password"
                type="password"
                pattern="\S+"
                minlength="6"
                required
                placeholder="Mật khẩu"
              />
              <input type="submit" value="Thêm" style="margin-top: 10px" />
            </form>
          </div>
        </div>

        <!-- Model change password -->
        <div id="ChangePasswordModal" class="modal-add-device">
          <div class="modal-content-add">
            <span class="close-add-device" id="closeChangePassword"
              >&times;</span
            >
            <h2 style="margin-bottom: 18px">Đỗi mật khẩu</h2>
            <h4 id="changePasswordError" style="color: crimson"></h4>
            <h4 id="changePasswordSuccess" style="color: springgreen"></h4>
            <br />
            <form id="changePasswordForm" class="add-device-form">
              <input type="hidden" name="username" id="modalUsername" />
              <label>Old password</label>
              <input
                name="old-password"
                type="password"
                required
                placeholder="Mật khẩu cũ"
              />
              <label>New Password</label>
              <input
                name="new-password"
                type="password"
                required
                placeholder="Mật khẩu mới"
              />
              <label>Pre-New Password</label>
              <input
                name="pre-new-password"
                type="password"
                required
                placeholder="Xác nhận mật khẩu mới"
              />
              <input type="submit" value="Change" style="margin-top: 10px" />
            </form>
          </div>
        </div>
      </main>
)rawliteral";

const char manage_pin_html_start[] PROGMEM = R"rawliteral(
      <main id="pins-content" style="display: none">
        <h1 style="justify-self: center">PIN Management</h1>
        <table
          class="rwd-table"
          style="justify-self: center; min-width: 50%; text-align-last: center"
        >
          <tr>
            <th>PIN</th>
            <th>Status</th>
            <th>Action</th>
            <th>Delete</th>
          </tr>
)rawliteral";

const char manage_pin_html_end[] PROGMEM = R"rawliteral(
          <tr>
            <td></td>
            <td></td>
            <td></td>
            <td>
              <button
                id="btnAddPin"
                style="
                  margin: 20px 0 10px 0;
                  padding: 10px 24px;
                  background: #3c91e6;
                  color: #fff;
                  border: none;
                  border-radius: 8px;
                  font-size: 16px;
                  cursor: pointer;
                  transition: background 0.2s;
                "
              >
                + Thêm PIN
              </button>
            </td>
          </tr>
        </table>

        <!-- Modal Thêm PIN -->
        <div id="addPinModal" class="modal-add-device">
          <div class="modal-content-add">
            <span class="close-add-device" id="closeAddPin">&times;</span>
            <h2 style="margin-bottom: 18px">Thêm PIN</h2>
            <form method="POST" action="/add-pin" class="add-device-form">
              <label>PIN</label>
              <input
                name="pin"
                type="text"
                required
                placeholder="Nhập PIN"
                maxlength="12"
              />
              <label>Status</label>
              <select name="status" required>
                <option value="active">Active</option>
                <option value="inactive">Inactive</option>
              </select>
              <input type="submit" value="Thêm" style="margin-top: 10px" />
            </form>
          </div>
        </div>
      </main>
    </section>
)rawliteral";

const char manage_footer_html[] PROGMEM = R"rawliteral(
  <script>
      document
        .querySelector(".confirm-restart")
        .addEventListener("click", function (e) {
          e.preventDefault(); // chặn chuyển trang ngay lập tức
          Swal.fire({
            title: "Bạn có chắc chắn?",
            text: "ESP sẽ restart ngay lập tức.",
            icon: "warning",
            showCancelButton: true,
            confirmButtonColor: "#3085d6",
            cancelButtonColor: "#d33",
            confirmButtonText: "Restart ngay",
            cancelButtonText: "Hủy",
          }).then((result) => {
            if (result.isConfirmed) {
              window.location.href = this.href; 
            }
          });
        });

      document
        .querySelector(".confirm-reset")
        .addEventListener("click", function (e) {
          e.preventDefault();
          Swal.fire({
            title: "Xác nhận Reset?",
            text: "Mạng WiFi sẽ bị xóa, bạn cần cấu hình lại.",
            icon: "warning",
            showCancelButton: true,
            confirmButtonColor: "#3085d6",
            cancelButtonColor: "#d33",
            confirmButtonText: "Reset ngay",
            cancelButtonText: "Hủy",
          }).then((result) => {
            if (result.isConfirmed) {
              window.location.href = this.href;
            }
          });
        });
    </script>  
  <script>
      document.addEventListener("DOMContentLoaded", function () {
        const btnAddPortNote = document.getElementById("btnAddPortNote");
        const modalPortNote = document.getElementById("addPortNoteModal");
        const closePortNote = document.getElementById("closeAddPortNote");
        const formPortNote = document.getElementById("formAddPortNote");
        const errorPortNote = document.getElementById("addPortNoteError");

        if (btnAddPortNote && modalPortNote && closePortNote) {
          btnAddPortNote.addEventListener("click", function () {
            modalPortNote.classList.add("show");
          });
          closePortNote.addEventListener("click", function () {
            modalPortNote.classList.remove("show");
            errorPortNote.innerHTML = "";
            formPortNote.reset();
          });
          window.addEventListener("click", function (event) {
            if (event.target === modalPortNote) {
              modalPortNote.classList.remove("show");
              errorPortNote.innerHTML = "";
              formPortNote.reset();
            }
          });
        }

        function isValidIP(ip) {
          const parts = ip.split(".");
          if (parts.length !== 4) return false;
          return parts.every((part) => {
            if (!/^\d+$/.test(part)) return false;
            const num = parseInt(part, 10);
            return num >= 0 && num <= 255;
          });
        }

        if (formPortNote) {
          formPortNote.addEventListener("submit", function (e) {
            let errors = [];
            const ip = this.ip_nat.value.trim();
            const internalPortStart = parseInt(
              this.internal_port_start.value.trim(),
              10
            );
            const internalPortEnd = parseInt(
              this.internal_port_end.value.trim(),
              10
            );
            const forwardedPortStart = parseInt(
              this.forwarded_port_start.value.trim(),
              10
            );
            const forwardedPortEnd = parseInt(
              this.forwarded_port_end.value.trim(),
              10
            );

            if (!isValidIP(ip)) {
              errors.push("IP NAT không hợp lệ. (ví dụ: 10.10.10.252)");
            }
            if (
              isNaN(internalPortStart) ||
              internalPortStart < 1 ||
              internalPortStart > 65535
            ) {
              errors.push(
                "Internal Port Start phải nằm trong khoảng 1 - 65535"
              );
            }
            if (
              isNaN(internalPortEnd) ||
              internalPortEnd < 1 ||
              internalPortEnd > 65535
            ) {
              errors.push("Internal Port End phải nằm trong khoảng 1 - 65535");
            }
            if (internalPortEnd < internalPortStart) {
              errors.push("Internal Port End phải lớn hơn hoặc bằng Start");
            }
            if (
              isNaN(forwardedPortStart) ||
              forwardedPortStart < 1 ||
              forwardedPortStart > 65535
            ) {
              errors.push(
                "Forwarded Port Start phải nằm trong khoảng 1 - 65535"
              );
            }
            if (
              isNaN(forwardedPortEnd) ||
              forwardedPortEnd < 1 ||
              forwardedPortEnd > 65535
            ) {
              errors.push("Forwarded Port End phải nằm trong khoảng 1 - 65535");
            }
            if (forwardedPortEnd < forwardedPortStart) {
              errors.push("Forwarded Port End phải lớn hơn hoặc bằng Start");
            }
            if (errors.length > 0) {
              e.preventDefault();
              errorPortNote.innerHTML = errors.join("<br />");
            }
          });
        }
      });
    </script>
    <script>
      const modal = document.getElementById("ChangePasswordModal");
      const closeBtn = document.getElementById("closeChangePassword");
      const form = document.getElementById("changePasswordForm");
      const usernameInput = document.getElementById("modalUsername");
      const errorMsg = document.getElementById("changePasswordError");

      // Mở modal khi bấm Change
      document.querySelectorAll(".btn-change").forEach((btn) => {
        btn.addEventListener("click", () => {
          const username = btn.dataset.username;
          usernameInput.value = username;
          modal.style.display = "flex";
        });
      });

      closeBtn.addEventListener("click", () => {
        modal.style.display = "none";
      });

      window.addEventListener("click", (e) => {
        if (e.target === modal) {
          modal.style.display = "none";
        }
      });

      form.addEventListener("submit", (e) => {
        e.preventDefault();

        const data = new FormData(form);
        let error = [];
        if (
          data.get("old-password").includes(" ") ||
          data.get("new-password").includes(" ")
        ) {
          error.push("Mật khẩu không được chứa khoảng trắng");
        }
        if (data.get("old-password") === data.get("new-password")) {
          error.push("Mật khẩu mới không được trùng với mật khẩu cũ");
        }
        if (data.get("new-password") !== data.get("pre-new-password")) {
          error.push("Mật khẩu mới không trùng khớp");
        }

        if (
          data.get("new-password").length < 6 ||
          data.get("old-password").length < 6
        ) {
          error.push("Mật khẩu phải có ít nhất 6 ký tự");
        }

        if (error.length > 0) {
          errorMsg.innerHTML = error.join("<br />");
          return;
        }

        errorMsg.innerText = "";

        axios
          .post("/change-password", data)
          .then((response) => {
            if (response.status == 200) {
              console.log("Response:", response);
              modal.style.display = "none";
              form.reset();
              $("#noti").html(
                "<h4 style='color: green'>Đổi mật khẩu thành công</h4>"
              );
              setTimeout(() => {
                $("#noti").html("");
              }, 5000);
              errorMsg.innerText = "";
            } else {
              errorMsg.innerText =
                response.data.message || "Lỗi không xác định";
            }
          })
          .catch((error) => {
            console.error("Error changing password:", error);
            errorMsg.innerText = "Lỗi khi kết nối đến server";
          });

        setTimeout(() => {
          modal.style.display = "none";
        }, 3000);
      });
    </script>

    <script>
      function isValidMAC(mac) {
        const macRegex = /^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$/;
        return macRegex.test(mac);
      }

      function isValidIP(ip) {
        const parts = ip.split(".");
        if (parts.length !== 4) return false;
        return parts.every((part) => {
          if (!/^\d+$/.test(part)) return false;
          const num = parseInt(part, 10);
          return num >= 0 && num <= 255;
        });
      }

      document
        .querySelector(".add-device-form")
        .addEventListener("submit", function (e) {
          const name = this.name.value.trim();
          const mac = this.mac.value.trim();
          const ip = this.ip.value.trim();
          const broadcast = this.broadcast.value.trim();
          const port = parseInt(this.port.value.trim(), 10);

          let errors = [];

          if (name.length === 0) {
            errors.push("Tên thiết bị không được để trống");
          }

          if (!isValidMAC(mac)) {
            errors.push("MAC không hợp lệ. Định dạng: XX:XX:XX:XX:XX:XX");
          }

          if (!isValidIP(ip)) {
            errors.push("IP không hợp lệ. (ví dụ: 192.168.1.100)");
          }

          if (!isValidIP(broadcast)) {
            errors.push("Broadcast không hợp lệ. (ví dụ: 255.255.255.255)");
          }

          if (isNaN(port) || port < 1 || port > 65535) {
            errors.push("Port phải nằm trong khoảng 1 - 65535");
          }

          if (errors.length > 0) {
            e.preventDefault();
            $("#addDeviceError").html(errors.join("<br />"));
            // alert(errors.join("\n"));
          }
        });
    </script>

    <script>
      document.addEventListener("DOMContentLoaded", function () {
        // Hiển thị pins-content khi click menu
        const mainContent = document.getElementById("mainContent");
        const devicesContent = document.getElementById("devices-content");
        const usersContent = document.getElementById("users-content");
        const pinsContent = document.getElementById("pins-content");
        const menuLinks = document.querySelectorAll(
          "#sidebar .side-menu.top li a"
        );

        menuLinks.forEach((link) => {
          link.addEventListener("click", function () {
            if (this.textContent.trim() === "PIN Management") {
              mainContent.classList.remove("show");
              devicesContent.classList.remove("show");
              usersContent.style.display = "none";
              pinsContent.style.display = "block";
            } else {
              pinsContent.style.display = "none";
            }
          });
        });

        const btnAddPin = document.getElementById("btnAddPin");
        const modalPin = document.getElementById("addPinModal");
        const closePin = document.getElementById("closeAddPin");
        if (btnAddPin && modalPin && closePin) {
          btnAddPin.addEventListener("click", function () {
            modalPin.classList.add("show");
          });
          closePin.addEventListener("click", function () {
            modalPin.classList.remove("show");
          });
          window.addEventListener("click", function (event) {
            if (event.target === modalPin) {
              modalPin.classList.remove("show");
            }
          });
        }
      });
    </script>

    <script>
      document.addEventListener("DOMContentLoaded", function () {
        const mainContent = document.getElementById("mainContent");
        const devicesContent = document.getElementById("devices-content");
        const usersContent = document.getElementById("users-content");
        const menuLinks = document.querySelectorAll(
          "#sidebar .side-menu.top li a"
        );

        menuLinks.forEach((link) => {
          link.addEventListener("click", function () {
            if (this.textContent.trim() === "Dashboard") {
              mainContent.classList.add("show");
              devicesContent.classList.remove("show");
              usersContent.style.display = "none";
            } else if (this.textContent.trim() === "My Device") {
              mainContent.classList.remove("show");
              devicesContent.classList.add("show");
              usersContent.style.display = "none";
            } else if (this.textContent.trim() === "User Management") {
              mainContent.classList.remove("show");
              devicesContent.classList.remove("show");
              usersContent.style.display = "block";
            }
          });
        });

        const btnAddUser = document.getElementById("btnAddUser");
        const modalUser = document.getElementById("addUserModal");
        const closeUser = document.getElementById("closeAddUser");
        if (btnAddUser && modalUser && closeUser) {
          btnAddUser.addEventListener("click", function () {
            modalUser.classList.add("show");
          });
          closeUser.addEventListener("click", function () {
            modalUser.classList.remove("show");
          });
          window.addEventListener("click", function (event) {
            if (event.target === modalUser) {
              modalUser.classList.remove("show");
            }
          });
        }
      });
    </script>

    <script>
      const allSideMenu = document.querySelectorAll(
        "#sidebar .side-menu.top li a"
      );

      allSideMenu.forEach((item) => {
        const li = item.parentElement;

        item.addEventListener("click", function () {
          allSideMenu.forEach((i) => {
            i.parentElement.classList.remove("active");
          });
          li.classList.add("active");
        });
      });

      const menuBar = document.querySelector("#content nav .bx.bx-menu");
      const sidebar = document.getElementById("sidebar");

      menuBar.addEventListener("click", function () {
        sidebar.classList.toggle("hide");
      });

      function adjustSidebar() {
        if (window.innerWidth <= 576) {
          sidebar.classList.add("hide");
          sidebar.classList.remove("show");
        } else {
          sidebar.classList.remove("hide");
          sidebar.classList.add("show");
        }
      }

      window.addEventListener("load", adjustSidebar);
      window.addEventListener("resize", adjustSidebar);

      const switchMode = document.getElementById("switch-mode");
      switchMode.addEventListener("change", function () {
        if (this.checked) {
          document.body.classList.add("dark");
        } else {
          document.body.classList.remove("dark");
        }
      });

      document.addEventListener("DOMContentLoaded", function () {
        document.getElementById("mainContent").classList.add("show");
        document.body.classList.add("dark");
      });
    </script>
    
    <script>
      document.addEventListener("DOMContentLoaded", function () {
        const btnAddDevice = document.getElementById("btnAddDevice");
        const modal = document.getElementById("addDeviceModal");
        const closeBtn = document.getElementById("closeAddDevice");

        btnAddDevice.addEventListener("click", function () {
          modal.classList.add("show");
        });
        closeBtn.addEventListener("click", function () {
          modal.classList.remove("show");
        });
        window.addEventListener("click", function (event) {
          if (event.target === modal) {
            modal.classList.remove("show");
          }
        });

        const allSideMenu = document.querySelectorAll(
          "#sidebar .side-menu.top li a"
        );
        const queryString = window.location.search;
        const urlParams = new URLSearchParams(queryString);
        const tab = urlParams.get("tab");
        // Ẩn tất cả nội dung
        document.getElementById("mainContent").classList.remove("show");
        document.getElementById("devices-content").classList.remove("show");
        document.getElementById("users-content").style.display = "none";
        document.getElementById("pins-content").style.display = "none";

        // Xóa active ở tất cả SideMenu
        allSideMenu.forEach((item) => item.classList.remove("active"));

        // Hiện tab theo yêu cầu và set active
        if (tab === "devices") {
          document.getElementById("devices-content").classList.add("show");
          document
            .querySelector('[data-tab="devices"]')
            .classList.add("active");
        } else if (tab === "users") {
          document.getElementById("users-content").style.display = "block";
          document.querySelector('[data-tab="users"]').classList.add("active");
        } else if (tab === "pins") {
          document.getElementById("pins-content").style.display = "block";
          document.querySelector('[data-tab="pins"]').classList.add("active");
        } else {
          document.getElementById("mainContent").classList.add("show");
          document
            .querySelector('[data-tab="dashboard"]')
            .classList.add("active");
        }
        
        document.querySelectorAll("[data-tab]").forEach((btn) => {
          btn.addEventListener("click", () => {
            const tab = btn.dataset.tab;
            const url = window.location.pathname + "?tab=" + tab;
            history.pushState({}, "", url);
          });
        });

      });
    </script>
  </body>
  </html>
)rawliteral";

bool checkLogin(const String& username, const String& password) {
  auto it = std::find_if(users.begin(), users.end(), [&](const User& u) {
    return u.user_login == username && u.pass_login == password;
  });
  return it != users.end();
}

bool checkPIN(const String& pin_wol) {
  auto it = std::find_if(pins.begin(), pins.end(), [&](const Pin& p) {
    return p.pin_wol == pin_wol && p.status == true;
  });
  return it != pins.end();
}

bool checkUserExists(const String& username) {
  auto it = std::find_if(users.begin(), users.end(), [&](const User& u) {
    return u.user_login == username;
  });
  return it != users.end();
}

bool checkAuth() {
  IPAddress loggedInIP = server.client().remoteIP();
  for(int i = 0; i < MAX_SESSIONS; i++){
    if(sessions[i].loggedInIP == loggedInIP){
        return true;
    }
  }

  server.sendHeader("Location", "/login");
  server.send(303);
  return false;
}

bool isValidMAC(String mac) {
    int count = 0;
    for (int i = 0; i < mac.length(); i++) {
        if (isxdigit(mac[i])) { 
            count++;
        } else if (mac[i] == ':') {
            if (count != 2 && count != 0) return false;
             count = 0;
        } else {
           return false;
        }
    }
    return count == 2;
}

bool isValidIP(String ipStr) {
    IPAddress ip;
    return ip.fromString(ipStr);
}

void logoutSession() {
  if(!checkAuth()) return;
  IPAddress clientIP = server.client().remoteIP();
  for(int i = 0; i < MAX_SESSIONS; i++){
    if(sessions[i].loggedInIP == clientIP){
        sessions[i].loggedInIP = INADDR_NONE;
        sessions[i].lastActive = 0;
        break;
    }
  }
}

void sendWOL(String mac, String broadcast, int port) {
  if(!isValidMAC(mac)) {
    Serial.println("ERROR: Invalid MAC address format!");
    return; 
  }

  byte macBytes[6];
  macBytes[0] = strtol(mac.substring(0, 2).c_str(), NULL, 16);
  macBytes[1] = strtol(mac.substring(3, 5).c_str(), NULL, 16);
  macBytes[2] = strtol(mac.substring(6, 8).c_str(), NULL, 16); 
  macBytes[3] = strtol(mac.substring(9, 11).c_str(), NULL, 16);
  macBytes[4] = strtol(mac.substring(12, 14).c_str(), NULL, 16);
  macBytes[5] = strtol(mac.substring(15, 17).c_str(), NULL, 16);
  
  byte magicPacket[102];
  for (int i = 0; i < 6; i++) {
    magicPacket[i] = 0xFF;
  }
    
  for(int i = 6; i < 102; i += 6){
    for(int j = 0; j < 6; j++){
        magicPacket[i + j] = macBytes[j];
    }
  }
      
    IPAddress broadcastAddr;
    if(broadcast.equals("255.255.255.255")){
        broadcastAddr =  IPAddress(255,255,255,255);
    } else {
        broadcastAddr.fromString(broadcast);
    }

  udp.beginPacket(broadcastAddr, port);
  udp.write(magicPacket, 102);
  udp.endPacket();
}

void loadForwardedNote() {
  if (!SPIFFS.begin()) return;

  File file = SPIFFS.open("/forwarded_notes.json", "r");
  if (!file) return;

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println("Failed to parse JSON");
    return;
  }

  forwarded_notes.clear();
  for (JsonObject obj : doc.as<JsonArray>()) {
    ForwardedNote fn;
    fn.name = obj["name"].as<String>();
    fn.internal_port = obj["internal_port"].as<String>();
    fn.forwarded_port = obj["forwarded_port"].as<String>();
    fn.ip_nat = obj["ip_nat"].as<String>();
    fn.description = obj["description"].as<String>();
    forwarded_notes.push_back(fn);
  }
  file.close();
}

void saveForwardedNote() {
  DynamicJsonDocument doc(2048);
  for (auto &fn : forwarded_notes) {
    JsonObject obj = doc.createNestedObject();
    obj["name"] = fn.name;
    obj["internal_port"] = fn.internal_port;
    obj["forwarded_port"] = fn.forwarded_port;
    obj["ip_nat"] = fn.ip_nat;
    obj["description"] = fn.description;
  }

  File file = SPIFFS.open("/forwarded_notes.json", "w");
  if (!file) return;
  serializeJson(doc, file);
  file.close();
}

void loadPINs() {
  if (!SPIFFS.begin()) return;

  File file = SPIFFS.open("/pins.json", "r");
  if (!file) return;

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println("Failed to parse JSON");
    return;
  }

  pins.clear();
  for (JsonObject obj : doc.as<JsonArray>()) {
    Pin p;
    p.pin_wol = obj["pin_wol"].as<String>();
    p.status = obj["status"].as<bool>();;  
    pins.push_back(p);
  }
  file.close();
}

void savePINs() {
  DynamicJsonDocument doc(2048);
  for (auto &p : pins) {
    JsonObject obj = doc.createNestedObject();
    obj["pin_wol"] = p.pin_wol;
    obj["status"] = p.status;
  }

  File file = SPIFFS.open("/pins.json", "w");
  if (!file) return;
  serializeJson(doc, file);
  file.close();
}

void loadUsers() {
  if (!SPIFFS.begin()) return;

  File file = SPIFFS.open("/users.json", "r");
  if (!file) return;

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println("Failed to parse JSON");
    return;
  }

  users.clear();
  for (JsonObject obj : doc.as<JsonArray>()) {
    User u;
    u.user_login = obj["user_login"].as<String>();
    u.pass_login = obj["pass_login"].as<String>();;  
    users.push_back(u);
  }
  file.close();
}

void saveUsers() {
  DynamicJsonDocument doc(2048);
  for (auto &u : users) {
    JsonObject obj = doc.createNestedObject();
    obj["user_login"] = u.user_login;
    obj["pass_login"] = u.pass_login;
  }

  File file = SPIFFS.open("/users.json", "w");
  if (!file) return;
  serializeJson(doc, file);
  file.close();
}

void loadDevices() {
  if (!SPIFFS.begin()) return;

  File file = SPIFFS.open("/devices.json", "r");
  if (!file) return;

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println("Failed to parse JSON");
    return;
  }

  devices.clear();
  for (JsonObject obj : doc.as<JsonArray>()) {
    Device d;
    d.name = obj["name"].as<String>();
    d.mac = obj["mac"].as<String>();
    d.ip = obj["ip"].as<String>();
    d.broadcast = obj["broadcast"].as<String>();
    d.port = obj["port"].as<int>();  
    devices.push_back(d);
  }
  file.close();
}

void saveDevices() {
  DynamicJsonDocument doc(2048);
  for (auto &d : devices) {
    JsonObject obj = doc.createNestedObject();
    obj["name"] = d.name;
    obj["mac"] = d.mac;
    obj["ip"] = d.ip;
    obj["broadcast"] = d.broadcast;
    obj["port"] = d.port;
  }

  File file = SPIFFS.open("/devices.json", "w");
  if (!file) return;
  serializeJson(doc, file);
  file.close();
}

void handleLogin() {
  if (server.method() == HTTP_POST) {
    String user = server.arg("username");
    String pass = server.arg("password");
    Serial.println(user);
    Serial.println(pass);
    if (checkLogin(user, pass)) {
      IPAddress loggedInIP = server.client().remoteIP();
      unsigned long now = millis();
      for(int i = 0; i < MAX_SESSIONS; i ++)
      {
        if(sessions[i].loggedInIP == INADDR_NONE){
          sessions[i].loggedInIP = loggedInIP;
          sessions[i].lastActive = now;
          server.send(200, "text/html; charset=utf-8", "success");
          return;
        }
      }
      server.send(403, "text/html; charset=utf-8;", "<h1>Max Sessions! - Vui lòng thử lại sau.</h1>");
    } else {
      server.send(403, "text/html; charset=utf-8", "");
    }
  } else {
    server.send(200, "text/html", login_html);
  }
}

void handleManage() {
  if (!checkAuth()) return;

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html; charset=utf-8", "");
  server.sendContent_P(manage_header_html);
  server.sendContent_P(manage_slidebar_html); 
  server.sendContent_P(manage_navbar_html); 

  server.sendContent_P(manage_dashboad_html_0);
  {
    String row = "<h3>" + String(devices.size()) + "</h3>";
    server.sendContent(row);
  }
  server.sendContent_P(manage_dashboad_html_1);
  {
    String row = "<h3>" + String(users.size()) + "</h3>";
    server.sendContent(row);
  }
  server.sendContent_P(manage_dashboad_html_2);
  for(int i = 0; i <forwarded_notes.size(); i++) {
    String row = "<tr>"
                  "<td>" + String(i+1) + "</td>"
                  "<td>" + forwarded_notes[i].name + "</td>"
                  "<td>" + forwarded_notes[i].internal_port + "</td>"
                  "<td>" + forwarded_notes[i].forwarded_port + "</td>"
                  "<td>" + forwarded_notes[i].ip_nat + "</td>"
                  "<td>" + forwarded_notes[i].description + "</td>"
                  "<td>"
                    "<a href=\"/delete-forwarded-note?id=" + String(i) + "\""
                      "><i class=\"bx bx-trash\" style=\"color: crimson\"></i"
                    "></a>"
                  "</td>"
                "</tr>";
    server.sendContent(row);
  }
  server.sendContent_P(manage_dashboad_html_3);

  server.sendContent_P(manage_device_html_start);
  for (int i = 0; i < devices.size(); i++) {
    String row = "<tr>"
                 "<td data-th='Device Name'>" + devices[i].name + "</td>"
                 "<td data-th='Device Mac'>" + devices[i].mac + "</td>"
                 "<td data-th='Device IP'>" + devices[i].ip + "</td>"
                 "<td data-th='Device Port WOL'>" + String(devices[i].port) + "</td>"
                 "<td data-th='Device Broadcast Address'>" + devices[i].broadcast + "</td>"
                 "<td data-th='Action'><a style='color: crimson' href='/delete-device?id=" + String(i) + "'>Xóa</a></td>"
                 "</tr>";
    server.sendContent(row);
  }
  server.sendContent_P(manage_device_html_end);
  
  server.sendContent_P(manage_user_html_start);
  if(users.size() == 1) {
    String row = "<tr>"
                  "<td data-th='Username'>" + users[0].user_login + "</td>"
                  "<td data-th='Password'>*******</td>"
                  "<td> <a data-username=\"" + users[0].user_login +"\"style=\"font-size: x-large; color: #cfe8ff\" class=\"bx bx-edit btn-change\" href=\"#\"></a></td>"
                  "<td data-th=\"Action\">No action</td>"
                  "</tr>";
      server.sendContent(row);
  }else {
    for (int i = 0; i < users.size(); i++) {
      String row = "<tr>"
                  "<td data-th='Username'>" + users[i].user_login + "</td>"
                  "<td data-th='Password'>*******</td>"
                  "<td> <a data-username=\"" + users[i].user_login +"\"style=\"font-size: x-large; color: #cfe8ff\" class=\"bx bx-edit btn-change\" href=\"#\"></a></td>"
                  "<td data-th=\"Action\"> <a style=\"color: crimson; font-size: x-large\" class=\"bx bx-trash\" href=\"/delete_user?user=admin\" ></a> </td>"
                  "</tr>";
      server.sendContent(row);
    }
  }
  
  server.sendContent_P(manage_user_html_end);

  server.sendContent_P(manage_pin_html_start);
  for (int i = 0; i < pins.size(); i++) {
    String status = pins[i].status ? "Active" : "Inactive";
    String newStatus = pins[i].status ? "inactive" : "active";
    String row = "<tr>"
                 "<td data-th='PIN'>" + pins[i].pin_wol + "</td>"
                 "<td data-th='Status' style=\"color:" + (status ? "green" : "crimson") + "\">" + status + "</td>"
                 "<td data-th='Action'><a href='/change-pin-status?pin=" + pins[i].pin_wol + "&status=" + newStatus + "' style=\"color:gold\"> Change status </a></td>"
                 "<td data-th='Delete'><a style='color: crimson' href='/delete-pin?pin=" + pins[i].pin_wol + "'>Xóa</a></td>"
                 "</tr>";
    server.sendContent(row);
  }
  server.sendContent_P(manage_pin_html_end);

  server.sendContent_P(manage_footer_html); 
  server.sendContent("");
  // server.send(200, "text/html; charset=utf-8", managePage());
}

void handleAddDevice() {
  if (!checkAuth()) return;
  Device d;
  if(isValidMAC(server.arg("mac")) == false) {
    server.send(400, "text/html; charset=utf-8", "Địa chỉ MAC không hợp lệ!");
    return;
  }
  if(isValidIP(server.arg("ip")) == false) {
    server.send(400, "text/html; charset=utf-8", "Địa chỉ IP không hợp lệ!");
    return;
  }
  if(isValidIP(server.arg("broadcast")) == false) {
    server.send(400, "text/html; charset=utf-8", "Địa chỉ Broadcast không hợp lệ!");
    return;
  }
  d.name = server.arg("name");
  d.mac = server.arg("mac");
  d.ip = server.arg("ip");
  d.broadcast = server.arg("broadcast");
  d.port = server.arg("port").toInt();
  devices.push_back(d);
  saveDevices();
  server.sendHeader("Location", "/manage?tab=devices");
  server.send(303);
}

void handleDeleteDevice() {
  if (!checkAuth()) return;
  int id = server.arg("id").toInt();
  if (id >= 0 && id < devices.size()) {
    devices.erase(devices.begin() + id);
    saveDevices();
  }
  server.sendHeader("Location", "/manage?tab=devices");
  server.send(303);
}

void handleAddUsers() {
  if (!checkAuth()) return;
  User u;
  if(checkUserExists(server.arg("username"))) {
    server.send(400, "text/html; charset=utf-8", "User đã tồn tại!");
    return;
  }
  
  if(server.arg("username").length() < 3 || server.arg("password").length() < 6) {
    server.send(400, "text/html; charset=utf-8", "Tên đăng nhập phải có ít nhất 3 ký tự và mật khẩu phải có ít nhất 6 ký tự!!");
    return;
  }
  u.user_login = server.arg("username");
  u.pass_login = server.arg("password");
  users.push_back(u);
  saveUsers();
  server.sendHeader("Location", "/manage?tab=users");
  server.send(303);
}

void handleDeleteUser() {
  if (!checkAuth()) return;
  if(users.size() <= 1) {
    server.send(400, "text/html; charset=utf-8", "Không thể xóa user cuối cùng!");
    return;
  }
  String username = server.arg("user");
  if(!checkUserExists(username)) {
    server.send(400, "text/html; charset=utf-8", "User không tồn tại!");
    return;
  }

  for (int i = 0; i < users.size(); i++) {
    if (users[i].user_login == username) {
      users.erase(users.begin() + i);
      saveUsers();
      break;
    }
  }
  server.sendHeader("Location", "/manage?tab=users");
  server.send(303);
}

void handleChangePassword() {
  String username = server.arg("username");
  String oldPassword = server.arg("old-password");
  String newPassword = server.arg("new-password");
  for (auto &u : users) {
    if (u.user_login == username) {
      if(u.pass_login != oldPassword) {
        server.send(400, "text/html; charset=utf-8", "Mật khẩu cũ không đúng!");
        return;
      }
      u.pass_login = newPassword;
      saveUsers();
      break;
    }
  }

  // server.sendHeader("Location", "/manage?tab=users");
  server.send(200);
}

void handleAddPIN() {
  if (!checkAuth()) return;
  Pin p;
  for (auto &existingPin : pins) {
    if (existingPin.pin_wol == server.arg("pin")) {
      server.send(400, "text/html; charset=utf-8", "PIN đã tồn tại!");
      return;
    }
  }
  if(server.arg("pin").length() < 4 || server.arg("pin").length() > 12) {
    server.send(400, "text/html; charset=utf-8", "PIN phải có từ 4 đến 12 ký tự!");
    return;
  }
  p.pin_wol = server.arg("pin");
  p.status = (server.arg("status") == "active");
  pins.push_back(p);
  savePINs();
  server.sendHeader("Location", "/manage?tab=pins");
  server.send(303);
}

void handleDeletePIN() {
  if (!checkAuth()) return;
  String pinToDelete = server.arg("pin");
  for (int i = 0; i < pins.size(); i++) {
    if (pins[i].pin_wol == pinToDelete) {
      pins.erase(pins.begin() + i);
      savePINs();
      break;
    }
  }
  server.sendHeader("Location", "/manage?tab=pins");
  server.send(303);
}

void handleChangePinStatus() {
  if (!checkAuth()) return;
  String pinToChange = server.arg("pin");
  String newStatus = server.arg("status");
  for (auto &p : pins) {
    if (p.pin_wol == pinToChange) {
      p.status = (newStatus == "active");
      savePINs();
      break;
    }
  }
  server.sendHeader("Location", "/manage?tab=pins");
  server.send(303);
}

void handleAddForwardedNote() {
  if (!checkAuth()) return;
  ForwardedNote fn;
  if(!isValidIP(server.arg("ip_nat"))) {
    server.send(400, "text/html; charset=utf-8", "Địa chỉ IP NAT không hợp lệ!");
    return;
  }
  fn.name = server.arg("name");
  fn.internal_port = server.arg("internal_port_start") + "-" + server.arg("internal_port_end");
  fn.forwarded_port = server.arg("forwarded_port_start") + "-" + server.arg("forwarded_port_end");
  fn.ip_nat = server.arg("ip_nat");
  fn.description = server.arg("description");
  forwarded_notes.push_back(fn);
  saveForwardedNote();
  server.sendHeader("Location", "/manage?tab=dashboard");
  server.send(303);
}

void handleDeleteForwardedNote() {
  if (!checkAuth()) return;
  int id = server.arg("id").toInt();
  if (id >= 0 && id < forwarded_notes.size()) {
    forwarded_notes.erase(forwarded_notes.begin() + id);
    saveForwardedNote();
  }
  server.sendHeader("Location", "/manage?tab=dashboard");
  server.send(303);
}

void handleEditForwardedNote() {
  if (!checkAuth()) return;
  int id = server.arg("id").toInt();
   if(!isValidIP(server.arg("ip_nat"))) {
      server.send(400, "text/html; charset=utf-8", "Địa chỉ IP NAT không hợp lệ!");
      return;
    }
  if (id >= 0 && id < forwarded_notes.size()) {
    forwarded_notes[id].name = server.arg("name");
    forwarded_notes[id].internal_port = server.arg("internal_port");
    forwarded_notes[id].forwarded_port = server.arg("forwarded_port");
    forwarded_notes[id].ip_nat = server.arg("ip_nat");
    forwarded_notes[id].description = server.arg("description");
    saveForwardedNote();
  }
  server.sendHeader("Location", "/manage?tab=dashboard");
  server.send(303);
}

void handleSendWOL() {
    String password = server.arg("password");  

    if (!checkPIN(password)) {
        server.send(400, "text/plain", "Mật khẩu không đúng!");
        Serial.println("Incorrect password");
        return; 
    }

    Serial.println("Start handleSendWOL");

    String selectedDeviceName = server.arg("device");
    Serial.print("Selected Device Name: ");
    Serial.println(selectedDeviceName);
    
    String selectedBroadcast = server.arg("broadcast");
    Serial.print("Selected Broadcast: ");
    Serial.println(selectedBroadcast);

    String selectedPortStr = server.arg("port");
    Serial.print("Selected Port String: ");
    Serial.println(selectedPortStr);
    int selectedPort = selectedPortStr.toInt();
    Serial.print("Selected Port int: ");
    Serial.println(selectedPort);

    int deviceIndex = -1;
    for (int i = 0; i < devices.size(); i++) {
        if (devices[i].name == selectedDeviceName) {
            deviceIndex = i;
            break;
        }
    }
    if (deviceIndex == -1) {
       server.send(400, "text/plain", "Không tìm thấy thiết bị");
       Serial.println("Device not found!");
       return;
    }

    Device selectedDevice = devices[deviceIndex];
    Serial.print("Selected Device MAC: ");
    Serial.println(selectedDevice.mac);
    Serial.print("Selected Device broadcast: ");
    Serial.println(selectedDevice.broadcast);

    Serial.println("Sending WOL...");
    sendWOL(selectedDevice.mac, selectedDevice.broadcast, selectedDevice.port);

    server.send(200, "text/plain", "Đã gửi gói tin WOL");

    Serial.println("End handleSendWOL");
}

void handleRoot() {
    String html = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">"
                  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                  "<link rel=\"icon\" href=\"https://cdn-icons-png.freepik.com/512/6329/6329326.png\" type=\"image/png\">"
                  "<title>WOL Controller</title>"
                  "<style>"
                  "body { font-family: Arial, sans-serif; text-align: center; padding: 20px; background-color: #f4f4f9; }"
                  ".device-container { display: flex; justify-content: center; flex-wrap: wrap; gap: 20px; }"
                  ".device { width: 150px; height: 150px; border: 2px solid #ccc; border-radius: 10px; display: flex; flex-direction: column; justify-content: center; align-items: center; cursor: pointer; background-color: #ffffff21; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); }"
                  ".device:hover { background-color: #e8e8e8; }"
                  ".device img { width: 80px; height: 80px; margin-bottom: 10px; }"
                  ".device-name { font-size: 18px; font-weight: bold; }"
                  ".modal { display: none; position: fixed; z-index: 10; left: 0; top: -50px; width: 100%; height: 110%; background-color: rgba(0,0,0,0.5); justify-content: center; align-items: center; }"
                  ".modal-content { background: #e3dfc8; padding: 20px; border-radius: 10px; text-align: center; width: 300px; }"
                  ".modal-content input { width: 90%; padding: 10px; margin: 10px 0; font-size: 16px; border: 1px solid #ccc; border-radius: 5px; }"
                  ".modal-content button { padding: 10px 20px; font-size: 16px; background-color: #007BFF; color: white; border: none; border-radius: 5px; cursor: pointer; }"
                  ".modal-content button:hover { background-color: #0056b3; }"
                  ".spinner { border: 3px solid #f3f3f3; border-top: 3px solid #3498db; border-radius: 50%; width: 16px; height: 16px; animation: spin 1s linear infinite; display: inline-block; vertical-align: middle; margin-left: 5px; }"
                  "@keyframes spin { 0%   { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }"
                  "</style>"
                  "</head><body style=\""
                  "background-image: url(https://marketplace.canva.com/EAFHyccmd3I/1/0/1600w/canva-brown-and-green-illustration-desktop-wallpaper-uFoN-4UuMEk.jpg);"
                  "background-repeat: no-repeat;"
                  "background-size: cover;\">"
                  "<h1 style=\"font-size: 48px; margin-bottom: 50px;\">WOL Controller</h1>"
                  "<div class=\"device-container\">";

    // Tạo icon cho từng thiết bị
    for (int i = 0; i < devices.size(); i++) {
        const char* imageUrl;

        // Kiểm tra nếu tên chứa "PC"
        if (strstr(devices[i].name.c_str(), "PC") != NULL) {
            imageUrl = "https://cdn-icons-png.flaticon.com/512/9711/9711115.png"; 
        }
        // Kiểm tra nếu tên chứa "SERVER"
        else if (strstr(devices[i].name.c_str(), "SERVER") != NULL) {
            imageUrl = "https://cdn-icons-png.flaticon.com/512/1508/1508901.png"; 
        } else {
            imageUrl = "https://cdn-icons-png.flaticon.com/512/9711/9711115.png"; 
        }
      
        html += "<div class=\"device\" onclick=\"openModal('" + devices[i].name + "', '" + devices[i].broadcast + "', '" + devices[i].port + "')\">"
                "<img src=\"" + imageUrl + "\" alt=\"Device Icon\">" 
                "<div class=\"device-name\">" + devices[i].name + "</div>"
                "</div>";
    }

    html += "</div>"
            "<div style=\"font-size:26px;margin-top:10px;font-weight:bold;display:none;\" id=noti>Thông báo:</div>"


            // Popup modal
            "<div id=\"modal\" class=\"modal\">"
            "<div class=\"modal-content\">"
            "<h2>Nhập Mật Khẩu</h2>"
            "<input type=\"password\" id=\"passwordInput\" placeholder=\"Mật khẩu\">"
            "<p style=\"color:red;font-size:16px;display:none;\" id=errorEmtyPass>Vui Lòng Nhập Password</p>"
            "<p style=\"color:red;font-size:16px;display:none;\" id=errorNoti>Mật khẩu không chính xác</p>"
            "</br>"
            "<button id=\"sendBtn\" style=\"margin-top:5px;\" onclick=\"sendWOL()\">Gửi</button>"
            "<button onclick=\"closeModal()\" style=\"background-color: #ccc; margin-left: 10px;\">Hủy</button>"
            "</div>"
            "</div>"

            "<script>"
            "let selectedDevice = {};"

            "function openModal(device, broadcast, port) {"
            "  const btn = document.getElementById('sendBtn');"
            "  selectedDevice = { device, broadcast, port };"
            "  document.getElementById('modal').style.display = 'flex';"
            "  document.getElementById('passwordInput').focus();"
            "  btn.disabled = false; btn.innerHTML = \"Gửi\";"
            "  document.getElementById('errorEmtyPass').style.display=\"none\";"
            "  document.getElementById('errorNoti').style.display = \"none\";"
            "}"

            "function closeModal() {"
            "  const btn = document.getElementById('sendBtn');"
            "  document.getElementById('modal').style.display = 'none';"
            "  document.getElementById('passwordInput').value = '';"
            "  btn.disabled = false; btn.innerHTML = \"Gửi\";"
            "}"

            "function sendWOL() {"
            "  const btn = document.getElementById('sendBtn');"
            "  btn.disabled = true;"
            "  btn.innerHTML = 'Đang gửi <span class=\"spinner\"></span>';"
            "  document.getElementById('errorEmtyPass').style.display=\"none\";"
            "  document.getElementById('errorNoti').style.display = \"none\";"
            "  document.getElementById('noti').style.display = \"none\";"
            "  const password = document.getElementById('passwordInput').value;"
            "  if (!password) { document.getElementById('errorEmtyPass').style.display = \"inline\"; document.getElementById('passwordInput').focus(); btn.disabled = false; btn.innerHTML = \"Gửi\"; return; }"
            "  const formData = new FormData();"
            "  formData.append('device', selectedDevice.device);"
            "  formData.append('broadcast', selectedDevice.broadcast);"
            "  formData.append('port', selectedDevice.port);"
            "  formData.append('password', password);"
            "  fetch('/sendWOL', { method: 'POST', body: formData })"
            "    .then(response => {"
            "      if (response.status == 400) {"
            "        document.getElementById('errorNoti').style.display = \"inline\";"
            "        document.getElementById('passwordInput').focus();"
            "        btn.disabled = false; btn.innerHTML = \"Gửi\";"
            "        return;"
            "      } else if (response.ok) {"
            "        document.getElementById('noti').style.display = \"block\";"
            "       closeModal();"
            "      }"
            "      return response.text();"
            "    })"
            "    .then(text => {"
            "      if (text){ document.getElementById('noti').innerText=text;console.log(\"Server response:\", text);}"
            "    })"
            "    .catch(error => {"
            "      console.error(\"Fetch error:\", error);"
            "    });"
            " }"

            // Lắng nghe sự kiện bàn phím khi modal mở
            "document.addEventListener('keydown', function (event) {"
            "  const modal = document.getElementById('modal');"
            "  if (modal.style.display === 'flex') {"
            "    if (event.key === 'Enter') {"
            "      sendWOL();"
            "    } else if (event.key === 'Escape') {"
            "      closeModal();"
            "    }"
            "  }"
            "});"
            "</script>"
            "</body></html>";

    server.send(200, "text/html", html);
}

void init_service(){
  Serial.println("Initializing service...");
  Serial.println("Loading User...");
  loadUsers();
  Serial.println("Loading PIN...");
  loadPINs();
  Serial.println("Loading Device...");
  loadDevices();
  Serial.println("Loading Forwarded Note...");
  loadForwardedNote();
  bool isDoneUser = true;
  bool isDonePin = true;
  if(users.size() == 0) {
    Serial.println("No users found, creating default user.");
    User admin;
    admin.user_login = DEFAULT_USER;
    admin.pass_login = DEFAULT_PASS;
    users.push_back(admin);
    saveUsers();
    isDoneUser = false;
    Serial.println("Default user created:");
    Serial.print("Username: ");Serial.println(DEFAULT_USER);
    Serial.print("Password: "); Serial.println(DEFAULT_PASS);
  }

  if(pins.size() == 0) {
    Serial.println("No PINs found, creating default PIN.");
    Pin defaultPin;
    defaultPin.pin_wol = DEFAULT_PIN;
    defaultPin.status = true;
    pins.push_back(defaultPin);
    savePINs();
    isDonePin = false;
    Serial.println("Default PIN created:");
    Serial.print("PIN: ");Serial.println(DEFAULT_PIN);
  }

  if (!isDoneUser) {
    Serial.println("Reloading users...");
    loadUsers();
  }

  if (!isDonePin) {
    Serial.println("Reloading PINs...");
    loadPINs();
  }
}

void setup() {

  Serial.begin(115200);
  WiFi.hostname("ESP8266_WOL");

  if (!wm.autoConnect("ESP_WOL_Config", "04102004")) {
    Serial.println("Kết nối thất bại hoặc hết thời gian chờ");
    ESP.restart();
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  for (int i = 0; i < MAX_SESSIONS; i++) {
    sessions[i].loggedInIP = INADDR_NONE;
    sessions[i].lastActive = 0;
  }

  init_service();
  server.on("/", handleRoot);
  server.on("/sendWOL", HTTP_POST, handleSendWOL);
  server.on("/login", handleLogin);
  server.on("/manage", handleManage);

  server.on("/add-device", HTTP_POST, handleAddDevice);
  server.on("/delete-device", handleDeleteDevice);

  server.on("/add-user", HTTP_POST, handleAddUsers);
  server.on("/delete-user", handleDeleteUser);
  server.on("/change-password", HTTP_POST, handleChangePassword);

  server.on("/add-pin", HTTP_POST, handleAddPIN);
  server.on("/delete-pin", handleDeletePIN);
  server.on("/change-pin-status", handleChangePinStatus);

  server.on("/add-forwarded-note", HTTP_POST, handleAddForwardedNote);
  server.on("/delete-forwarded-note", handleDeleteForwardedNote);
  server.on("/edit-forwarded-note", HTTP_POST, handleEditForwardedNote);

  server.on("/logout", []() {
    logoutSession();
    server.sendHeader("Location", "/login");
    server.send(303);
  });

  server.on("/esp-restart", []() {
    if(checkAuth()){
      Serial.println("Restarting device...");
      server.sendHeader("Location", "/login");
      server.send(303);
      ESP.restart();
    }   
  });

  server.on("/hard-reset", []() {
    if(checkAuth()){
      Serial.println("Resetting device...");
      server.sendHeader("Location", "/login");
      server.send(303);
      wm.resetSettings();
      ESP.restart();
    }
  });

  server.begin();
  Serial.println("Web server started");

  lastResetTime = millis();
}

void loop() {
  server.handleClient();

  for(int i = 0; i < MAX_SESSIONS; i++){
    if(millis() - sessions[i].lastActive >= 300000){
        sessions[i].loggedInIP = INADDR_NONE;
        sessions[i].lastActive = 0;
    }
  }

  // Kiểm tra và khởi động lại sau mỗi tiếng
  if (millis() - lastResetTime >= 3600000) {
    Serial.println("Restarting device...");
    ESP.restart();
  }
}
