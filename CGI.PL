# -*-Perl-*-
# some perl procedures for dealing with cgi scripts
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


sub parse_query {
	local (*data, $uppercase, *raw)=@_;
	local ($RequestMethod, $InputMax, $p, $ContentLength, $i);
	# determine calling method
	$RequestMethod = $ENV{"REQUEST_METHOD"};
	$InputMax = 4096;

	if ($RequestMethod eq "POST") {

	    $p = $ENV{"CONTENT_LENGTH"}; # determine a useable content length
	    if ($p) {
		$ContentLength = $p;
	    } else {
		$ContentLength = 0;
	    }

	    # limit the content length by the maxInput
	    if ($ContentLength> $InputMax-1) {
		$ContentLength = $InputMax-1;
	    }


	    # read the content into a string
	    $i = 0;
	    while ($i<$ContentLength) {
		$InputBuffer .= getc;
		$i++;
	    }

	    $ContentLength = $i;   # determine the actual (read) length

	    # get the content type
	    $p = $ENV{"CONTENT_TYPE"};
	    if (!$p) {
		die "Couldn't find CONTENT_TYPE\n";
	    }

	    if ($p eq "application/x-www-form-urlencoded") {
		&parse_parms(*data, $InputBuffer, $uppercase, *raw);
	    } 

	} elsif ($RequestMethod eq "GET") {

	    local ($query) = $ENV{'QUERY_STRING'};
	    &parse_parms(*data, $query, $uppercase, *raw);
	}
}

sub parse_parms {

	local (*data, $tmpString, $uppercase, *raw)=@_;
	local ($wrk, @tmp, $x, @t);

	$wrk = $tmpString;

	# replace any %dd sequences with the Correct Perl Escapes
	#
	$wrk =~ s/%(\w\w)/\\x$1/g;

	# translates +'s to spaces
	#
	$wrk =~ s/\+/ /g;

        # just in case we had to enclose in in quotes to
        # get by the brain damaged NT command prompt...
        $wrk =~ s/^\"//g;
        $wrk =~ s/\"$//g;

	# parse the query elements into an array
	#
	@tmp = split(/&/, $wrk);

	foreach $x (@tmp) {

	    @t = split(/=/, $x);
	    if ($uppercase) {
		$t[0] =~ tr/a-z/A-Z/;
	    }
	    $t[1] =  eval("\"$t[1]\""), "<P>";
	    $data{$t[0]} = $t[1];

	}	

	@tmp = split(/&/, $tmpString);
	foreach $x (@tmp) {
	    @t = split(/=/, $x);
            $raw{$t[0]} = $t[1];
        }
}

1;





