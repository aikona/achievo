<?php

function getThemes() 
{      
  $s = array();
  
  // First look in the themes directory      
  $handle=opendir("themes");
  while ($file = readdir($handle)) 
    if ($file != "." && $file != ".." && $file != "CVS")  $s[]= $file;
  closedir($handle); 
    
  // Then look in the atk backend directory
  $handle=opendir("atk/themes");
  while ($file = readdir($handle)) 
    if ($file != "." && $file != ".." && $file != "CVS") $s[]= $file;
  closedir($handle); 
    
  sort($s);
    
  // return array with themes
  return $s;
}

useattrib("atkdummyattribute");
useattrib("atkpasswordattribute");
useattrib("atklistattribute");
    
class userprefs extends atkNode
{
  function userprefs()
  {
    $this->atkNode("userprefs",NF_NO_ADD|NF_NO_DELETE); 

    $this->add(new atkAttribute("userid",AF_PRIMARY|AF_HIDE));
    if (atkconfig("authentication")=="db")
    {
      $this->add(new atkDummyAttribute("passwordexplanation", text("password_leaveblankifunchanged"), AF_HIDE_ADD|AF_HIDE_LIST));
      $this->add(new atkPasswordAttribute("password",AF_OBLIGATORY));
    }
    $themes = getThemes();
    $this->addAttribute(new atkListAttribute("theme",$themes,$themes,AF_HIDE_ADD));      
    $this->setTable("employee");      
  }

  function descriptor_def()
  {
    return "";
  }
  
  function postUpdate($record)
  {
    global $g_user;
  
    if($record["userid"]==$g_user["name"])
    {
      echo '<script language="javascript">';
      echo 'parent.location = "index.php"';
      echo '</script>';
      exit;
    }
  }
}   
?>