#!/usr/bin/perl
# Copyright (C) 1995, Brian C. Jepson
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

package iodbc;
sub new {
    my $self = {};
    bless $self;
    $self;
}

#
# create a query and save it in a temp file
#
sub createquery {

    my $self = shift;
    $self->{'query'} = $_[0];

    # if I already defined/created a tmpfile, then I should delete it
    if (defined($self->{'tmpfile'})) {
	unlink($self->{'tmpfile'});
    }

    #
    # create a temporary file name derived using my pid
    #
    $self->{'tmpfile'} = "sql$$" . time() . ".sql";

    #
    # create/open that file for writing
    #
    open(SQL_FILE, ">$self->{'tmpfile'}") || 
	die "Can't create $tmpfile\n";

    select(SQL_FILE); $| = 1;		# make it unbuffered
    select(STDOUT);

    #
    # exec the query and GO!
    #
    print SQL_FILE "$self->{'query'}\n";
    print SQL_FILE "GO\n";
    
    close (SQL_FILE);
	
}

sub sqlexec {
	
    my $self = shift;
    my($dsn, $user, $password)=@_;
    #
    # Open the pipe from ISQL and execute a query in the cmdfile
    #
    #open(SQL_HANDLE, "iodbc.exe /z /S \"$dsn\" /n /s \"~\" < $self->{'tmpfile'}|") || die "can't open pipe to isql"; 
    open(SQL_HANDLE, qq[type $self->{'tmpfile'} | iodbc.exe /U "$user" /P "$password" /z /S "$dsn" /n /w 4096 /s "~" |]) || die "can't open pipe to isql"; 

    sqlqueryparse $self;
    close(SQL_HANDLE);

    unlink($self->{'tmpfile'});
}

sub sqlqueryparse {

    my $self = shift;

    #
    # we are now on the line with the headers. We'll
    # parse it using whitespace as the delimiter
    #
    # note that we are removing the first ~, since it screws
    # parsing up
    #
    ($x = <SQL_HANDLE>);

        $x =~ s/^~//;

    if ($x =~ /^Msg/) {
	chop $x;
	$self->{'error'} = 1;
	$self->{'error'}{'number'} = $x;
	$errorinfo = <SQL_HANDLE>;

	$errorinfo =~ /^\[(.*)\]\[(.*)\](.*)/;
	$self->{'error'}{'vendor'}  = $1;
	$self->{'error'}{'product'} = $2;
	$self->{'error'}{'message'} = $3;

    }
    else {
	my(@headers) = split(/\s*~\s*/, $x);
	foreach $item (@headers) {

	    $self->{'headers'}[$icount++] = $item;

	}

	# skip the dashed line
	#
	$x = <SQL_HANDLE>;

	#
	# process each row and insert it into the array
	#
	$recno = 0;

	while ( ($value1 = <SQL_HANDLE>) =~ /\S/){  	# until we hit a line 
							# w/ no whitespace
	    $recno++;
	    ($line = $value1) =~ s/^~//;
	    @rrow = split(/\s*~\s*/, $line);
	    $i = 0;
	    $hashdef = '';
	    foreach $index (@headers) {
		$self->{'results'}[$recno]{$index} = $rrow[$i++];
	    }
	}
    }

}



sub printresults {

    my $self = shift;
    local ($quiet, *array) = @_;
    my ($i);
    if (!(error $self)) {

	if (defined @{$self->{'results'}}) {

	    $reccount = $#{$self->{'results'}};
	    for ($i = 1; $i <= $reccount; $i++) {

		print "RECORD $i\n" unless $quiet;
		foreach $junk (keys %{$self->{'results'}[$i]}) {
		    print "$junk = $self->{'results'}[$i]{$junk}\n" unless $quiet;
		    @array[$i] = $self->{'results'}[$i]{$junk};
		}
	    }
       	} else {
            print "This query returned no data.\n" unless $quiet;
        }
    }

}


sub error {

    my $self = shift;
    my ($message);
    if ($self->{'error'}) {
	$message .= 
	    qq[This query is in an error state.\n];
	$message .= 
	    qq[The query that produced the error was: "$self->{'query'}"\n];
	$message .= 
	    qq[The error was produced by the $self->{'error'}{'vendor'} $self->{'error'}{'product'}.\n];

	$message .= qq[The error message is $self->{'error'}{'message'}\n];
	$message .= qq[($self->{'error'}{'number'})\n];
	return $message;
    } else {
	0;	
    }	

}

1;



