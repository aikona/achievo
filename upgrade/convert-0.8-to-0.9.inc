<?php

  $g_db->query("CREATE TABLE schedule (
                   id int(10) unsigned DEFAULT '0' NOT NULL,
                   startdate date DEFAULT '0000-00-00' NOT NULL,
                   enddate date DEFAULT '0000-00-00' NOT NULL,
                   starttime time DEFAULT '00:00:00' NOT NULL,
                   endtime time DEFAULT '00:00:00' NOT NULL,
                   title varchar(50) NOT NULL,
                   description text,
                   location varchar(50) DEFAULT '0',
                   allday tinyint(1) DEFAULT '0' NOT NULL,
                   publicitem tinyint(1) DEFAULT '0' NOT NULL,
                   owner varchar(15) NOT NULL,
                   showowner tinyint(1) DEFAULT '0' NOT NULL,
                   scheduletype int(10) DEFAULT '0' NOT NULL,
                   PRIMARY KEY (id)   
                )");      

  $g_db->query("CREATE TABLE schedule_attendees (
                   scheduleid int(10) DEFAULT '0' NOT NULL,
                   userid varchar(15) NOT NULL,
                   PRIMARY KEY (scheduleid, userid)   
                )");      
   
  $g_db->query("CREATE TABLE schedule_types (
                   id int(10) unsigned DEFAULT '0' NOT NULL,
                   description varchar(50) NOT NULL,
                   bgcolor varchar(7) NOT NULL,
                   fgcolor varchar(7) NOT NULL,
                   PRIMARY KEY (id)
                )");

?>