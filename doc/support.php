<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html dir="ltr" lang="en">
  <head>
    <meta http-equiv="Content-Type" content="text/html;charset=utf-8" />
    <title>#unreal-support Quiz</title>
    <style type="text/css">
      <!--
        body {margin: 10px; padding: 10px; width: 95%; color: #000000; font-family: Verdana, Arial, Helvetica, sans-serif; font-size: 11px; background-color: #FFFFFF;}
        a:link, a:visited {text-decoration: none; color: #5C66A0;}
        a:hover {text-decoration: none; color: #000000;}

        input {background-color: #FFFFFF; color: #333333; border: 1px solid #000000; font-size: 11px; font-weight: bold;}
      -->  
    </style>
    <script type="text/javascript">
      var _gaq = _gaq || [];
     _gaq.push(['_setAccount', 'UA-18644081-2']);
     _gaq.push(['_trackPageview']);

     (function() {
       var ga = document.createElement('script'); ga.type = 'text/javascript'; ga.async = true;
       ga.src = ('https:' == document.location.protocol ? 'https://ssl' : 'http://www') + '.google-analytics.com/ga.js';
       var s = document.getElementsByTagName('script')[0]; s.parentNode.insertBefore(ga, s);
     })();

   </script>
 </head>
 <body>
  <div>In order to receive support in #unreal-support, you must take and pass a quiz.<br />This quiz is designed to ensure you have read the documentation _before_ asking in the help channel. This first page contains some very general questions.</div>

  <form action="/quiz.php" method="post">
   <div style="display: none;"><input type="hidden" name="page" value="1" /></div>
   <ol>
    <li>What OS do you run Unreal on?<br />
    <input type="radio" name="questions[os]" value="win" />Windows<br />
    <input type="radio" name="questions[os]" value="nix" />*nix<br />
    <input type="radio" name="questions[os]" value="mac" />Mac</li>
    <br />
    <li>What version of Unreal do you use?<br />
    <input type="radio" name="questions[ver]" value="316" />3.1.6<br />
    <input type="radio" name="questions[ver]" value="32" />3.2.8.1 (or newer)<br />
    <!-- <input type="radio" name="questions[ver]" value="4" />4.0<br /> -->
    <input type="radio" name="questions[ver]" value="other" />Other</li>
    <br />
    <li>Did you, or anyone else modify the source code in your copy of Unreal (NOT the conf)?<br />
    <input type="radio" name="questions[mod]" value="1" />Yes<br />
    <input type="radio" name="questions[mod]" value="0" />No</li>
   </ol>
   <div><input type="submit" value="Continue" style="margin-left: 40px;" /></div>
  </form>

 </body>
</html>
 
