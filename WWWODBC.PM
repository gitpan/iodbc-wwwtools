# -*-Perl-*-
#

#####
# This class holds all of the resuable elements
# of an object which would use ODBC in a WWW query
#####
package wwwodbc;

use iodbc;
use html;
require "cgi.pl";

sub new {
    my $self = {};
    bless $self;

    local(%cgiQuery);
    &parse_query(*cgiQuery);

    $self->{'DSN'}      = $cgiQuery{'DSN'};
    $self->{'USER'}     = $cgiQuery{'USER'};
    $self->{'PASSWORD'} = $cgiQuery{'PASSWORD'};
    $self->{'CGIQUERY'} = %cgiQuery;
    $self->{'PAGE'}     = new htmlpage;

    $self;
}

sub sql {
    my $self = shift;
    my $sql  = shift;

    my $query = new iodbc;
    createquery $query $sql;
    sqlexec     $query ($self->{'DSN'}, $self->{'USER'}, $self->{'PASSWORD'});

    if (defined @{$query->{'results'}}) {
	@{$self->{'SQLRESULTS'}} = @{$query->{'results'}};
    } else {
	($error = error $query) =~ s/\n/<br>/g;;
	addobject {$self->{'PAGE'}} ("html_text", $error);	
    }
}

sub addobject {
    my $self = shift;
    addobject {$self->{'PAGE'}} @_;
}

sub show {
    my $self = shift;
    show {$self->{'PAGE'}};
}
