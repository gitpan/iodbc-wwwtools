# -*- Perl -*-
#
# sql.pl
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


$seconds = time();

&parse_query(*results, 0, *raw); # parse the query

# set up default delimiters for each type of data
# I don't need any for numeric data, and I haven't
# added any other data types...
#
$delim{'CHAR', 'LEFT'}  = "'\\'";
$delim{'CHAR', 'RIGHT'} = "\\''";


open (TEMPLATE, "<$results{'TEMPLATE'}") ;
while (<TEMPLATE>) {
   chop;
   $sql .= $_;
   last if (/;$/);
}

while (<TEMPLATE>) {
   chop;
   ($field, $type) = split(/\=/, $_);
   $type =~ s/;$//;
   $datatypes{$field} = $type;
   last if (/;$/);
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
$sql = eval(qq["$sql"]);

$dsn = $results{'DSN'};		# get the data source name
				
$query = new iodbc;		# create and exec the iodbc query
createquery $query $sql;
sqlexec $query ($dsn, $results{'USER'}, $results{'PASSWORD'});                      

$persist = 'DSN=' . $raw{'DSN'} . '&USER=' . $raw{'USER'} . '&PASSWORD=' . $raw{'PASSWORD'};

while (<TEMPLATE>) {
   ($head .= $_) =~ s/;$//;
   last if (/;$/);
}

while (<TEMPLATE>) {
   ($template .= $_) =~ s/;$//;
   last if (/;$/);
}

while (<TEMPLATE>) {
   ($footer .= $_) =~ s/;$//;
   last if (/;$/);
}

close(TEMPLATE);

print "Content-type: text/html\n\n";

# process the output of the select statement.
@array = @{$query->{'results'}};

$reccount = $#array;
for ($i = 1; $i <= $reccount; $i++) {

   foreach $key (keys %{$array[$i]}) {
      $assignment = '$' . $key . " = '${$array[$i]}{$key}' ";
      eval($assignment);
   }
   $head = eval(qq["$head"]);
   print $head unless ($printed); $printed = 1;
   $html = eval(qq["$template"]);
   print $html;
}
$seconds = time() - $seconds;

$footer = eval(qq["$footer"]);


print $footer;



