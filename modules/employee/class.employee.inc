<?php
/**
 * This file is part of the Achievo distribution.
 * Detailed copyright and licensing information can be found
 * in the doc/COPYRIGHT and doc/LICENSE files which should be
 * included in the distribution.
 *
 * @package achievo
 * @subpackage employee
 *
 * @copyright (c)2008 Ibuildings B.V.
 * @license http://www.achievo.org/licensing Achievo Open Source License
 *
 * @version $Revision$
 * $Id$
 */

userelation("atkmanytoonerelation");
userelation("atkonetomanyrelation");
userelation("atkmanyboolrelation");
useattrib("atklistattribute");
useattrib("atkdummyattribute");
useattrib("atkpasswordattribute");
useattrib("atkdateattribute");
useattrib('employee.adminlangselectattribute');
useattrib('employee.contracttemplateattribute');

include_once (moduleDir("person")."class.person.inc");
include_once (moduleDir("utils")."class.dateutil.inc");

/**
 * Class for managing employees
 * @package achievo
 * @subpackage employee
 */
class employee extends person
{
  function employee()
  {
    global $ATK_VARS,$g_user;

    $this->person("employee", NF_EDITAFTERADD|NF_IMPORT);

    // In employee edit screen, we hide most fields in the add screen.
    foreach (array_keys($this->m_attribList) as $attribname)
    {
      if (!in_array($attribname, array("userid","lastname", "firstname","email")))
      {
        $p_attrib = &$this->getAttribute($attribname);
        $p_attrib->m_flags|=AF_HIDE_ADD;
      }
      elseif($attribname=="userid")
      {
        $p_attrib = &$this->getAttribute($attribname);
        $p_attrib->m_flags|=AF_OBLIGATORY;
      }
    }

    $this->addFilter("person.role='employee'");
    $this->add(new atkDateAttribute("birthdate","","", 0, date("Ymd"), AF_HIDE_LIST));

    $this->add(new atkAttribute("bankaccount", AF_HIDE_LIST|AF_HIDE_ADD), "finance");
    $this->add(new atkAttribute("socialsecuritynumber", AF_HIDE_LIST|AF_HIDE_ADD), "finance");

    $this->add(new atkManyToOneRelation("employer_id","organization.organization",AF_HIDE_ADD|AF_HIDE_LIST|AF_RELATION_AUTOLINK|AF_LARGE));
    $this->add(new atkManyToOneRelation("department","employee.department",AF_HIDE_ADD|AF_HIDE_LIST|AF_RELATION_AUTOLINK));

    // you can't be your own supervisor neither can a nonactive employee be a supervisor
    $preFilter = new atkManyToOneRelation("supervisor","employee.employee",AF_HIDE_ADD|AF_HIDE_LIST|AF_RELATION_AUTOLINK);
    $preFilter->setDestinationFilter("userid !='[userid]'");
    $preFilter->setUseFilterForAddLink(false);

    $this->add($preFilter);
      
    $this->add(new atkManyToOneRelation("functionlevel", "employee.functionlevel",AF_HIDE_ADD|AF_HIDE_LIST|AF_RELATION_AUTOLINK));

    //$this->add(new atkManyToOneRelation("entity", "employee.profile",AF_RELATION_AUTOLINK|AF_HIDE_ADD));
    $rel = &$this->add(new atkManyBoolRelation("profiles", "employee.employeerole", "employee.profile", AF_HIDE_LIST|AF_MANYBOOL_AUTOLINK), "account");
    $rel->setLocalKey("employee_id");
    $rel->setRemoteKey("role_id");

    // If we are in employee administration mode, we show all users. Otherwise, we only show active users.
    $this->add(new atkListAttribute("status", array("active","nonactive"), array(), AF_HIDE_LIST|AF_HIDE_ADD|AF_FORCE_LOAD), "account");
    if ((atkArrayNvl($ATK_VARS, "atknodetype", "") == "employee.employee") && in_array(atkArrayNvl($ATK_VARS, "atkaction", ""), array("admin", "add", "update", "edit")))
    {
      $statusfilter = ($this->getView() == "active") ? "active" : "nonactive";
      $this->addFilter("person.status",$statusfilter);
    }
    $this->add(new adminLangSelectAttribute('lng',AF_HIDE_LIST), "account");

    $this->add(new atkOneToManyRelation("usercontract","employee.usercontracts","userid",AF_HIDE_LIST|AF_HIDE_ADD));

    //$this->add(new useridAttribute("userid" ,AF_OBLIGATORY|AF_UNIQUE|AF_SEARCHABLE), 1); // first field

    if (in_array("db",explode(",",strtolower(atkconfig('authentication')))))
      $this->addPasswordAttribute();

    $this->add(new atkAttribute("role", AF_OBLIGATORY|AF_READONLY|AF_HIDE, 50)); // discriminator.
    $this->add(new contracttemplateAttribute("contract_template","employee.tpl_contract", AF_HIDE_LIST|AF_HIDE_EDIT|AF_FORCE_LOAD));

    $this->m_securityMap["deactivate"] = "edit"; // If you may edit, you may also deactivate.

    $this->setOrder("lastname");
    $this->setDescriptorTemplate("[lastname], [firstname] ([userid])");
  }

  function addPasswordAttribute()
  {
    $this->add(new atkDummyAttribute("passwordexplanation", atktext("password_leaveblankifunchanged"), AF_HIDE_ADD|AF_HIDE_LIST|AF_HIDE_SEARCH|AF_HIDE_VIEW), "account");
    $passwordrestrictions = array(
      "minsize" => atkconfig("password_minsize", 0),
      "minupperchars" => atkconfig("password_minupperchars", 0),
      "minlowerchars" => atkconfig("password_minlowerchars", 0),
      "minalphabeticchars" => atkconfig("password_minalphabeticchars", 0),
      "minnumbers" => atkconfig("password_minnumbers", 0),
      "minspecialchars" => atkconfig("password_minspecialchars", 0)
    );
    $this->add(new atkPasswordAttribute("password", false, AF_HIDE_LIST|AF_PASSWORD_NOVALIDATE, 40, $passwordrestrictions), "account");
  }

  function initial_values()
  {
    $initial = array();
    $initial["role"] = "employee";
    $initial["status"] = "active";

    return $initial;
  }

  /**
   * Are we in 'active' emps or 'archive' mode?
   */
  function getView()
  {
    global $g_sessionManager;
    $view = $g_sessionManager->stackVar("view");
    if ($view=="")
    {
      $view = "active";
    }
    return $view;
  }

  function action_admin($handler)
  {

    $view = $this->getView();
    if ($view=="active")
    {
      $this->addFilter("person.status","active");
    }
    return $handler->action_admin();
  }

  function adminFooter()
  {
    $view = $this->getView();

    if ($view=="active")
    {
      $ret = atktext("onlyactiverecordsareshown")." ".href(dispatch_url($this->atknodetype(),$this->m_action,array("view"=>"nonactive")),
                                                           atktext('clicktoviewarchive', $this->m_module, $this->m_type))."<br>";
    }
    else
    {
      $ret = atktext("youareinthearchive")." ".href(dispatch_url($this->atknodetype(),$this->m_action,array("view"=>"active")),
                                                    atktext('clicktoviewactiverecords', $this->m_module, $this->m_type))."<br>";
    }
    $ret.= '<br/>'.$this->getLegend();
    return $ret;
    
  }
  
  protected function getLegend()
  {
    $nrows = array();
    $nrows[] = array('description' => atkText('no_contract'),
                    'bgcolor' => COLOR_ERROR);
    $nrows[] = array('description' => atkText('contract_expires'),
                    'bgcolor' => COLOR_WARNING);
    
    $legend = '<div style="border: 1px solid black; background: white none repeat scroll 0%; -moz-background-clip: -moz-initial; -moz-background-origin: -moz-initial; -moz-background-inline-policy: -moz-initial; font-size: 1em;">
                 <dl><b>'.atktext("legend","scheduler").':</b></dl>
               <div style="margin-left: 2em;">';
    for ($i=0,$_i=count($nrows);$i<$_i;$i++)
    {
      $legend.='<p style="margin: 0px;">
                 <span style="border: medium none ; background-color: '.$nrows[$i]["bgcolor"].'; color: '.$nrows[$i]["bgcolor"].';">&nbsp;&nbsp;&nbsp;</span>
                   '.htmlspecialchars($nrows[$i]["description"]).'
                </p>';
    }
    $legend.='</div><br></div>';
    return $legend;
  }

  function recordActions($rec, &$actions, &$mraactions)
  {
    // Get the userid of the user currently logged in
    $user = atkGetUser();
    $userid = atkArrayNvl($user, "id", -1);

    // Actve users may be deactivated, but not the logged in user.
    if ($rec["status"] == "active" && $rec["id"]!=$userid)
    {
      $actions["deactivate"] = atkSessionManager::sessionUrl(dispatch_url($this->atkNodeType(),"deactivate",array("atkselector"=>"[pk]")), SESSION_NESTED);
    }

    // Prevent users from deleting themselves when they are logged in.
    if($rec["id"] == $userid)
    {
      unset($actions["delete"]);
    }
  }

  function action_deactivate(&$handler)
  {
    $select = $this->m_postvars["atkselector"];
    $query = "UPDATE person SET status='nonactive' WHERE $select";

    // Verify that the selector or id were not tampered with before passing on to the db
    if(preg_match("/person\.id='\d+'/i", $select) === 1)
    {
      $db = &atkGetDb();
      $db->query($query);
    }
    else
    {
      // TODO: give the name of the module, the logged userid and the IP address
      // of the offending user in the debug entry so it gets logged properly
      atkdebug("SQL injection attack detected in 'action_deactivate', not executed. The selector was '$select' and the resulting query would have been '$query'", true);
    }

    $this->redirect();
  }

  function graph_prjtime($params, $raw=false)
  {
    $db = &atkGetDb();
    $data = $db->getrows("SELECT
                              project.id, project.name, project.abbreviation, SUM(time) as total
                            FROM
                              hours, phase, project
                            WHERE
                              hours.phaseid = phase.id ".
                              ($params["employeeid"]!=""?" AND hours.userid = ".$params["employeeid"]:"")."
                              AND phase.projectid = project.id".
                              ($params["viewstart"]!=""?" AND hours.activitydate>='".$params["viewstart"]."'":"").
                              ($params["viewend"]!=""?" AND hours.activitydate<='".$params["viewend"]."'":"").
                          " GROUP BY
                              project.id,project.name
                            ORDER BY
                              project.name");

    if ($raw)
    {
      return $data;
    }

     // convert records to graph-compatible array.
    $dataset = array();
    for ($i=0, $_i=count($data); $i<$_i; $i++)
    {
      $key = $data[$i]["abbreviation"].": ".$data[$i]["name"];
      $dataset[$key] = $data[$i]["total"];
    }

    return array("registeredtimeperproj"=>$dataset);
  }

  function graph_activitytime($params, $raw=false)
  {
    $db = &atkGetDb();
    $data = $db->getrows("SELECT
                              activity.id, activity.name, SUM(time) as total
                            FROM
                              hours, activity
                            WHERE
                              hours.activityid = activity.id ".
                              ($params["viewstart"]!=""?" AND hours.activitydate>='".$params["viewstart"]."'":"").
                              ($params["viewend"]!=""?" AND hours.activitydate<='".$params["viewend"]."'":"").
                          ($params["employeeid"]!=""?" AND hours.userid = ".$params["employeeid"]:"")."
                            GROUP BY
                              activity.name
                            ORDER BY
                              activity.name");
    // In raw mode the data is sufficient.
    if ($raw) return $data;

    // convert records to graph-compatible array.
    $dataset = array();

    for ($i=0, $_i=count($data); $i<$_i; $i++)
    {
      $dataset[$data[$i]["name"]] = $data[$i]["total"];
    }

    return array("registeredtimeperactivity"=>$dataset);
  }

  function graph_dowtime($params, $raw=false)
  {
    $db = &atkGetDb();
    $days = array("sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday");

    $data = $db->getrows("SELECT
                              date_format(hours.activitydate, '%w') as dow, SUM(time) as total
                            FROM
                              hours, activity
                            WHERE
                              hours.activityid = activity.id ".
                              ($params["viewstart"]!=""?" AND hours.activitydate>='".$params["viewstart"]."'":"").
                              ($params["viewend"]!=""?" AND hours.activitydate<='".$params["viewend"]."'":"").
                              ($params["employeeid"]!=""?" AND hours.userid = ".$params["employeeid"]:"")."
                            GROUP BY
                              date_format(hours.activitydate, '%w')
                            ORDER BY
                              dow");

    // Add weekday names.
    for ($i=0, $_i=count($data); $i<$_i; $i++)
    {
      $data[$i]["dow"] = atktext($days[$data[$i]["dow"]]);
    }

    // In raw mode the data is sufficient.
    if ($raw) return $data;

    // convert records to graph-compatible array.
    $dataset = array();

    for ($i=0, $_i=count($data); $i<$_i; $i++)
    {
      $dataset[$data[$i]["dow"]] = $data[$i]["total"];
    }

    return array("registeredtimeperweekday"=>$dataset);
  }

  function rowColor($record)
  {
    if ($record["status"]=="active")
    {
      $contractnode = &atkGetNode("employee.usercontracts");

      $endlesscontracts = $contractnode->countDb("usercontract.enddate IS NULL AND usercontract.userid='{$record["id"]}'");

      if ($endlesscontracts == 0)
      {
        $contract = $contractnode->getMostRecentContract($record["id"]);

        $enddate = isset($contract["enddate"]) ? (dateutil::arr2str($contract["enddate"])) : "";

        if (!count($contract)||($enddate!=""&&$enddate<date("Ymd")))
        {
          return array(COLOR_ERROR, COLOR_HIGHLIGHT_ERROR);
        }
        else if ($enddate!=""&&dateutil::date_diff(date("Ymd"), $enddate)<14)
        {
          return array(COLOR_WARNING, COLOR_HIGHLIGHT_WARNING);
        }
      }
    }
  }

  function postAdd($record)
  {
    $pimitems = &atkGetNode("pim.userpimitems");
    $pimitems->addDefaultPimItemsForUser($record['id']);
    $this->removeBirthdaysCache();
    // Create scheduler userprefs
    $userprefs = &atkGetNode("scheduler.scheduler_userpreferences");
    $userprefs->createUserPreferences($record['id']);
    return true;
  }

  function postUpdate($record)
  {
    $this->removeBirthdaysCache();
    return true;
  }

  function postDel($record)
  {
    $this->removeBirthdaysCache();
    // Remove scheduler userprefs
    $userprefs = &atkGetNode("scheduler.scheduler_userpreferences");
    $userprefs->deleteDb("userid='".$record['id']."'");
    return true;
  }

  /**
   * Remove birthdays cache file
   */
  function removeBirthdaysCache()
  {
    atkimport("atk.utils.atktmpfile");
    $file = new atkTmpFile("employee_birthdays.inc");
    if($file->exists())
      $file->remove();
  }

  /**
   * Get birthdays from all employees
   *
   * @return array Array with all employees birthdays
   */
  function getBirthdates()
  {
    $birthdays = array();
    atkimport("atk.utils.atktmpfile");
    $file = new atkTmpFile("employee_birthdays.inc");
    if(!$file->exists())
    {
      $birthday_recs = $this->selectDb("birthdate IS NOT NULL AND status='active'","month(birthdate), dayofmonth(birthdate)","","",array("firstname","lastname","birthdate"));
      $items = array();
      foreach($birthday_recs as $birthday)
      {
        $day = intval($birthday["birthdate"]["day"]);
        $month = intval($birthday["birthdate"]["month"]);
        $name = $birthday["firstname"]." ".$birthday["lastname"];
        atkdebug("$day - $month - $name");
        $items[$month][$day][] = array("name"=>$name,"birthday"=>$birthday["birthdate"]);
      }

      $file->writeAsPhp("birthdays",$items);
    }
    else
    {
      include $file->getPath();
    }

    return $birthdays;
  }


}

?>
