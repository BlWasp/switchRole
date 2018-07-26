Authors
=======
Guillaume Daumas : guillaume.daumas@univ-tlse3.fr

Ahmad Samer Wazan : ahmad-samer.wazan@irit.fr



Intro
=====

This code implements a role based approach for distributing Linux capabilities into Linux users. It allows assigning Linux capabilities to Linux users without the need to inject the Linux capabilities into executable files. Our code is a PAM-based module that leverages a new capability set added to Linux kernel, called Ambient Set. Using this module, administrators can group a set of Linux capabilities in roles and give them to their users. For security reasons, users don’t get the attributed roles by default, they should activate them using the command sr (substitute role). Our module is compatible with PAM_CAP. So administrators can continue using PAM_CAP with our module.

Tested Platforms
===========
Our module has been tested only on Ubuntu and debian platforms.

Installation
===========

How to Build
------------

	1. git clone https://github.com/guillaumeDaumas/switchRole
    
    2. cd swithRole
    
    3. execute the following installation script as root :
		`sh ./buildSR.sh`
    
    4. restart your system.

Usage
-----

After the installation you will find a file called capabilityRole.conf in the /etc/security directory. You should configure this file in order to define the set of roles and assign them to users or group of users on your system.

Once configuration is done, a user can assume a role using the tool ‘sr’ that is installed with our package. In your shell type for example :

`./sr role1` 

After that a new shell is oppend that contains the capabilities in the role that has been taken by the user. You can verify by reading the capabilities of your shell (cat /proc/$$/status). When you exit you can retrun to your initial shell. 


References
==========

PAM repository : https://github.com/linux-pam/linux-pam

libcap repository : https://github.com/mhiramat/libcap



Very helpfull site, where you can find some informations about PAM, libcap and the capabilities:


Original paper about capabilities : https://pdfs.semanticscholar.org/6b63/134abca10b49661fe6a9a590a894f7c5ee7b.pdf

Article about the capabilities : https://lwn.net/Articles/632520/

Article about Ambient : https://lwn.net/Articles/636533/

Simple article with test code for Ambient : https://s3hh.wordpress.com/2015/07/25/ambient-capabilities/

Article about how PAM is working : https://artisan.karma-lab.net/petite-introduction-a-pam

A very helpfull code about how to create a PAM module : https://github.com/beatgammit/simple-pam
