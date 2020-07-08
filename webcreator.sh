#!/bin/bash

width=$(tput cols)   #terminal's width

if [ "$#" -ne 4 ]; then   #checking number of parameters
    echo "-! ERROR !- Illegal number of parameters"
    exit 1
fi


#checking if every command line argument has the proper format
if ! [ "$3" -eq "$3" ] 2>/dev/null
then
    echo "-! ERROR !- Argument 'w' is not an int" 
    exit 1
else
    if ! [ "$3" -ge "2" ]
    then
        echo "-! ERROR !- Argument 'w' must be greater than 1" 
        exit 1
    fi
fi

if ! [ "$4" -eq "$4" ] 2>/dev/null
then
    echo "-! ERROR !- Argument 'p' is not an int" 
    exit 1
else
    if ! [ "$4" -ge "2" ]
    then
        echo "-! ERROR !- Argument 'p' must be greater than 2" 
        exit 1
    fi
fi

w=$((10#$3))
p=$((10#$4))

root_dir=$1
text_file=$2

printf "\n==> Parameters given <==\n"
printf "=> Root directory: %s \n" $root_dir 
printf "=> Text file: %s \n" $text_file
printf "=> Number of web sites (w): %s \n" $w
printf "=> Number of pages per web site (p): %s \n" $p


if [ ! -d $1 ]; then
    echo "-! ERROR !- Root directory does not exist"
    exit 1;
fi

if [ ! -f $2 ]; then
    echo "-! ERROR !- Text file does not exist"
    exit 1;
fi

#checking text file's number of lines
n_lines=$(wc -l < $2)
printf "=> Text file's number of lines: %d\n" $n_lines
if ! [ $n_lines -ge 10000 ]
then
    echo "-! ERROR !- Text file must contain at least 10000 lines" 
    exit 2
fi

mapfile -t lines < $text_file    #get all text file's lines in an array

f=$(((p/2)+1))
printf "=> Number of internal links (f): %d\n" $f
        
q=$(((w/2)+1))
printf "=> Number of external links (q): %d\n" $q

html_nlines=$((f+q))
printf "=> Number of total links (f+q): %d\n" $html_nlines

up_bound=$((n_lines-2001))

n_pages=$((w * p))
printf "=> Number of total website pages: %d \n" $n_pages

for ((i=0; i<$width; i++)); 
do
    printf "=" 
done
printf "\n"

declare -A pages
declare -A inc_links   #indicates if links have been chosen or not (by index)
#init random pages' ids
for ((i=0; i<$w; i++)); 
do
    for ((j=0; j<$p; j++)); 
    do
        random_num=$(shuf -i 1-$n_pages -n 1)
        page_index=$random_num$i$j
        new_page="page"$i"_"$page_index".html"
        pages[$i,$j]=$new_page
        inc_links[$i,$j]=0      #init with zeros
    done
done

#checking if root directory is empty
if find $root_dir -mindepth 1 -print -quit | grep -q .; then
    printf "# Warning: root directory '%s' is full, purging ...\n" $root_dir
    del_dir=$root_dir"/*"
    #echo $del_dir
    rm -rf $del_dir
fi

for ((i=0; i<$w; i++)); 
do #for every website
    new_dir=$root_dir"/site"$i"/"
    printf "# Creating web site: '%d' with path: '%s'..\n" $i $new_dir
    mkdir -p $new_dir     #create new site directory
    
    for ((j=0; j<$p; j++)); 
    do #for every webpage
        new_page=$new_dir${pages[$i,$j]}
        k=$(shuf -i 2-$up_bound -n 1)
        m=$(shuf -i 1001-1999 -n 1)
        text_lines_per_html_line=$((m/(f+q)))
        printf "#  Creating page: '%s' with '%d' lines starting at line: '%d'...\n" $new_page $m $k
        touch $new_page   #create new webpage
            
#make random internal links=================================================================
        links=()  #stores all choosen links
        if [ $p -le 2 ]
        then  #if p is less than 2 then internal links will include the page itself
            for ((l=0; l<$f; l++));   
            do
                links+=(${pages[$i,$l]})
                inc_links[$i,$l]=1   #mark page as choosen link by index
            done
        else
            inter_links_indexes=()    #make a pool of pages' indexes to choose randomly as links
            inter_links_indexes_size=$((p-1))
            for ((l=0; l<$p; l++)); #add all website's pages indexes except for itself
            do
                if [ $l -ne $j ]
                then
                    inter_links_indexes+=($l)
                fi
            done
            
            for ((l=0; l<$f; l++));   #choose f random internal links by index
            do
                random_link_index=$(shuf -i 0-$((inter_links_indexes_size-1)) -n 1)
                link_index=${inter_links_indexes[$random_link_index]}
                inter_link=${pages[$i,$link_index]}
                links+=($inter_link)
                inc_links[$i,$link_index]=1  #mark as chosen link
                
                unset inter_links_indexes[$random_link_index]
                inter_links_indexes=( "${inter_links_indexes[@]}" )
                ((inter_links_indexes_size--))
            done
        fi
        
 
#make random external links =================================================================
#same as internal
        websites_indexes=()  #store all available websites to take pages from as links (by index)
        websites_indexes_size=$((w-1))
        declare -A external_links_taken
        for ((l=0;l<$w;l++)) 
        do #for every website
            if [ $l -ne $i ]
            then
                websites_indexes+=($l)  #keep  all available websites' indexes except for itself
            fi
            
            for ((z=0;z<$p;z++)) 
            do
                external_links_taken[$l,$z]=0
            done
        done
        
        for ((l=0;l<$q;l++)) 
        do # take q random external links
            random_website_index=$(shuf -i 0-$((websites_indexes_size-1)) -n 1) 
            website_index=${websites_indexes[$random_website_index]} #choose a random website index
            website_pages_indexes=()
            website_pages_indexes_size=0
            for ((z=0;z<$p;z++)) 
            do #check which pages of this site have not been chosen yet and keep their indexes at an array
                if [ ${external_links_taken[$website_index,$z]} -eq 0 ]
                then
                    website_pages_indexes+=($z) 
                    ((website_pages_indexes_size++))
                fi
            done
            
            if [ $website_pages_indexes_size -eq 1 ]
            then
                unset websites_indexes[$random_website_index]
                websites_indexes=( "${websites_indexes[@]}" )
                ((websites_indexes_size--))
            fi
            
            random_page_index=$(shuf -i 0-$((website_pages_indexes_size-1)) -n 1)
            page_index=${website_pages_indexes[$random_page_index]}
            
            external_links_taken[$website_index,$page_index]=1  #mark as chosen
            extern_link=${pages[$website_index,$page_index]}
            extern_link_path="../site"$website_index"/"$extern_link
            links+=($extern_link_path)
            inc_links[$website_index,$page_index]=1
        done
        
#===============================================================================     
        echo "<!DOCTYPE html>" >> $new_page
        echo "<html>" >> $new_page
        echo "  <body>" >> $new_page
        
        line_index=$k
        for ((l=0; l<$html_nlines; l++)); 
        do
            printf "    " >> $new_page
            
            for ((t=0; t<$text_lines_per_html_line; t++)); 
            do
                printf "%s " "${lines[$line_index]}" >> $new_page
                ((line_index++))
            done
          
            printf '\t<a href="%s">link%d_%s</a> <br>\n' ${links[$l]} $((l+1)) ${links[$l]} >> $new_page
          
        done
        
        echo "  </body>" >> $new_page
        echo "</html>" >> $new_page
        printf "#  Adding link to '%s'\n" $new_page
    done
    
    for ((j=0; j<$width; j++)); 
    do
        printf "=" 
    done
    printf "\n"
done

all_inc_links=1
for ((i=0; i<$w; i++)); 
do
    for ((j=0; j<$p; j++)); 
    do
        if [ ${inc_links[$i,$j]} -eq 0 ]
        then
            all_inc_links=0
            break
        fi
    done
done

if [ $all_inc_links -eq 1 ]
then
    printf "# All pages have at least one incoming link\n"
else
    printf "# NOT all pages have at least one incoming link\n"
fi
printf "# Done.\n"


