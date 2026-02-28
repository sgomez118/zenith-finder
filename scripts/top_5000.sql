/*
* This query gets the top brightest stars from The Strasbourg astronomical Data Center.
* The query is in ADQL, and it runs in the https://simbad.cds.unistra.fr/simbad-tap/
*/

SELECT TOP 5000 main_id,-- Main identifier for an object
                ra,-- Right ascension
                dec,-- Declination
                coo_qual,-- Coordinate quality
                pmra,-- Proper motion in RA
                pmdec,-- Proper motion in DEC
                pm_qual,-- Proper motion quality
                plx_value,-- Parallax
                plx_qual,-- Parallax quality
                rvz_radvel,-- Radial Velocity
                rvz_qual,-- Radial velocity / redshift quality
                flux,-- Magnitude/Flux
                flux_err,-- flux error
                qual,-- flux quality flag
                ids -- List of all identifiers concatenated with pipe
FROM   basic -- General data about an astronomical object
       JOIN flux -- Magnitude/Flux information about an astronomical object
         ON oid = flux.oidref
       JOIN ids
         ON basic.oid = ids.oidref
WHERE  otype = '*..' -- Object type
       AND filter = 'V' -- flux filter name
       AND ( flux_err < 0.1
              OR flux_err IS NULL )
ORDER  BY flux ASC;