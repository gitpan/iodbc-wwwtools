#!/usr/bin/perl
#
# sql.pl
#
# Embedded SQL support for HTML documents
#
# This script interfaces to Lee Fesperman's iodbc by way of
# iodbc.pm. Form variables are dynamically interpolated into the
# embedded SQL, providing the ability to insert SQL statements
# into the HTML document.
#
# Here's a little sample of a form which does an SQL INSERT:
# <he>This is an INSERT statement example</h1>
# <form action="cgi2perl.exe/sql.pl" method=post>
#
# <!-- this is the tag which contains the data source name -->
# <input type="hidden" name="DSN"       value="IODBC SAMPLE">
#
# <!-- this is the tag which contains datatype definitions -->
# <input type="hidden" name="DATATYPES"
#    value="first_name!CHAR,last_name!CHAR">
#
# <!-- this is the tag which contains the sql statement -->
# <input type="hidden" name="SQL" 
#    value="INSERT INTO PERSON (PE_FNAME, PE_LNAME)
#                VALUES ($first_name, $last_name)">
#
# <!-- now, we simply include the fields which will go into
# the database. they must correspond to perl variables in
# the select statement above!! -->
#
# First Name:<input type="text" name="first_name"><P>
# Last Name:<input type="text" name="last_name"><P>
#
#
# <!-- and, finally, a submit button to finish it all!!! -->
# <input type="SUBMIT">
# </form>
#
# Copyright 1995, Brian C. Jepson
#                 (bjepson@conan.ids.net)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#
use iodbc;			# the iodbc query object
require "cgi.pl";		# query parser
use html;

$page = new htmlpage;
$header = addobject $page ("html_header", 1, "Embedded SQL Results");

&parse_query(*results);		# parse the query
# set up default delimiters for each type of data
# I don't need any for numeric data, and I haven't
# added any other data types...
#
$delim{'CHAR', 'LEFT'}  = "'\\'";
$delim{'CHAR', 'RIGHT'} = "\\''";

# retrieve the datatype definitions from the DATATYPE variable
#
@datatypes=split(/,/, $results{'DATATYPES'});
foreach $line (@datatypes) {
    ($field, $type) = split(/!/, $line);
    $datatypes{$field} = $type;
}

# retrieve each variable which corresponds to a field in the
# datatype list and in the SQL statement
#
foreach $key (keys %results) {
    if ($key ne 'SQL' && $key ne 'DSN' && $key ne 'DATATYPES') {

	# get the delimiters according to the type
	# of the current field
	#
	$ldelim = $delim{$datatypes{$key}, 'LEFT'};
	$rdelim = $delim{$datatypes{$key}, 'RIGHT'};

	# create a variable with the same name as the 
	# form variable; this variable is interpolated
	# into the SQL statement later...
	#
	$assignment = "\$$key = $ldelim$results{$key}$rdelim";
	eval($assignment);
    }
}

# evaluate the SQL form variable, forcing interpolation on any variables
# which correspond to the variables we created above...
#

$sql = eval("\"$results{'SQL'}\"");

$dsn = $results{'DSN'};		# get the data source name
				
$query = new iodbc;		# create and exec the iodbc query
createquery $query $sql;
sqlexec $query ($dsn, $results{'USER'}, $results{'PASSWORD'});                      

# print out the query results in a table format...
#
if (defined @{$query->{'results'}}) {
	@array = @{$query->{'results'}};
	$null = addobject $page ("html_table", *array); 
} else {
   	$null = addobject $page ("html_text", "An error has occured which indicates that IODBC.EXE may not be installed properly...");
}

if ($query->{'error'}) {
	($message = error $query) =~ s/\n/<p>/g;
	$null = addobject $page ("html_text", $message);
	$null = addobject $page ("html_text", "The query which generated the error was:<BR>");
   	$null = addobject $page ("html_text", "<b>$sql</b><p>");
}

show $page;
