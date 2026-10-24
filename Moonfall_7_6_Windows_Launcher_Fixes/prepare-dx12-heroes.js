'use strict';
// Preserve originals byte-for-byte; convert only a COPY of Z's WebP to PNG for WIC.
const fs=require('node:fs');
const path=require('node:path');
const crypto=require('node:crypto');
const sharp=require('sharp');
const root=__dirname;
const assets=path.join(root,'modular','assets');
const output=path.resolve(root,'../native/dx12-prototype/build/Release');
const heroSources=[
 {name:'Z',source:'art-001.webp',target:'hero-z.png'},
 {name:'Nyra',source:'art-004.png',target:'hero-nyra.png'}
];
function sha(data){return crypto.createHash('sha256').update(data).digest('hex')}
async function buildHeroes(){
 fs.mkdirSync(output,{recursive:true});
 const manifest=[];
 for(const entry of heroSources){
  const source=path.join(assets,entry.source),target=path.join(output,entry.target);
  const original=fs.readFileSync(source),sourceHash=sha(original);
  if(entry.source.endsWith('.png')){
   fs.copyFileSync(source,target);
  }else{
   await sharp(original,{failOn:'error'}).png({compressionLevel:9}).toFile(target);
  }
  const decoded=await sharp(original).ensureAlpha().raw().toBuffer({resolveWithObject:true});
  const converted=await sharp(target).ensureAlpha().raw().toBuffer({resolveWithObject:true});
  if(decoded.info.width!==converted.info.width||decoded.info.height!==converted.info.height
     ||!decoded.data.equals(converted.data))throw Error('Character pixels changed during conversion: '+entry.name);
  if(sha(fs.readFileSync(source))!==sourceHash)throw Error('Original image changed: '+entry.name);
  const record={name:entry.name,source:entry.source,output:entry.target,
    width:decoded.info.width,height:decoded.info.height,sha256:sourceHash};
  manifest.push(record);
  console.log('Original '+entry.name+' art copied losslessly to '+entry.target+
   ' ('+record.width+'x'+record.height+', source SHA-256 '+sourceHash+')');
 }
 fs.writeFileSync(path.join(output,'original-hero-art.json'),JSON.stringify(manifest,null,2)+'\n');
}
if(require.main===module)buildHeroes().catch(err=>{console.error(err);process.exitCode=1});
module.exports={buildHeroes,heroSources};
