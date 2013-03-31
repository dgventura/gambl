
// Rar_Extractor 0.2.1. See unrar_license.txt.

#include "rar.hpp"

const char* Archive::ReadHeader()
{
	CurBlockPos = Tell();

	if ( OldFormat )
		return ReadOldHeader();

	RawRead Raw( rar_reader );

	Raw.Read( SIZEOF_SHORTBLOCKHEAD );
	if ( Raw.Size() == 0 )
	{
		Int64 ArcSize=FileLength();
		if (CurBlockPos>ArcSize || NextBlockPos>ArcSize)
			return "RAR file is missing data at the end";
		return end_of_rar;
	}

	Raw.Get(ShortBlock.HeadCRC);
	byte HeadType;
	Raw.Get(HeadType);
	ShortBlock.HeadType=(HEADER_TYPE)HeadType;
	Raw.Get(ShortBlock.Flags);
	Raw.Get(ShortBlock.HeadSize);
	if (ShortBlock.HeadSize<SIZEOF_SHORTBLOCKHEAD)
		return "CRC error in RAR file";

	if (ShortBlock.HeadType==COMM_HEAD)
		Raw.Read(SIZEOF_COMMHEAD-SIZEOF_SHORTBLOCKHEAD);
	else
		if (ShortBlock.HeadType==MAIN_HEAD && (ShortBlock.Flags & MHD_COMMENT)!=0)
			Raw.Read(SIZEOF_NEWMHD-SIZEOF_SHORTBLOCKHEAD);
		else
			Raw.Read(ShortBlock.HeadSize-SIZEOF_SHORTBLOCKHEAD);

	NextBlockPos=CurBlockPos+ShortBlock.HeadSize;

	switch(ShortBlock.HeadType)
	{
		case MAIN_HEAD:
			*(BaseBlock *)&NewMhd=ShortBlock;
			Raw.Get(NewMhd.HighPosAV);
			Raw.Get(NewMhd.PosAV);
			break;
		case ENDARC_HEAD:
			*(BaseBlock *)&EndArcHead=ShortBlock;
			if (EndArcHead.Flags & EARC_DATACRC)
				Raw.Get(EndArcHead.ArcDataCRC);
			if (EndArcHead.Flags & EARC_VOLNUMBER)
				Raw.Get(EndArcHead.VolNumber);
			break;
		case FILE_HEAD:
		case NEWSUB_HEAD:
			{
				FileHeader *hd=ShortBlock.HeadType==FILE_HEAD ? &NewLhd:&SubHead;
				*(BaseBlock *)hd=ShortBlock;
				Raw.Get(hd->PackSize);
				Raw.Get(hd->UnpSize);
				Raw.Get(hd->HostOS);
				Raw.Get(hd->FileCRC);
				Raw.Get(hd->FileTime);
				Raw.Get(hd->UnpVer);
				Raw.Get(hd->Method);
				Raw.Get(hd->NameSize);
				Raw.Get(hd->FileAttr);
				if (hd->Flags & LHD_LARGE)
				{
					Raw.Get(hd->HighPackSize);
					Raw.Get(hd->HighUnpSize);
				}
				else 
				{
					hd->HighPackSize=hd->HighUnpSize=0;
					if (hd->UnpSize==0xffffffff)
					{
						hd->UnpSize=int64to32(INT64MAX);
						hd->HighUnpSize=(INT64MAX>>32);
					}
				}
				hd->FullPackSize=int32to64(hd->HighPackSize,hd->PackSize);
				hd->FullUnpSize=int32to64(hd->HighUnpSize,hd->UnpSize);

				char FileName[NM*4];
				int NameSize=Min(hd->NameSize,sizeof(FileName)-1);
				Raw.Get((byte *)FileName,NameSize);
				FileName[NameSize]=0;

				strncpy(hd->FileName,FileName,sizeof(hd->FileName));
				hd->FileName[sizeof(hd->FileName)-1]=0;

				if (hd->HeadType==NEWSUB_HEAD)
				{
					int DataSize=hd->HeadSize-hd->NameSize-SIZEOF_NEWLHD;
					if (hd->Flags & LHD_SALT)
						DataSize-=SALT_SIZE;
					if (DataSize>0)
					{
						hd->SubData.Alloc(DataSize);
						Raw.Get(&hd->SubData[0],DataSize);
						if (hd->CmpName(SUBHEAD_TYPE_RR))
						{
							byte *D=&hd->SubData[8];
						}
					}
				}
				else
					if (hd->HeadType==FILE_HEAD)
					{
						hd->Flags &= ~LHD_UNICODE;
						
						if (NewLhd.UnpVer<20 && (NewLhd.FileAttr & 0x10))
							NewLhd.Flags|=LHD_DIRECTORY;
						
						if (NewLhd.HostOS>=HOST_MAX)
						{
							if ((NewLhd.Flags & LHD_WINDOWMASK)==LHD_DIRECTORY)
								NewLhd.FileAttr=0x10;
							else
								NewLhd.FileAttr=0x20;
						}
					}
				if (hd->Flags & LHD_SALT)
					Raw.Get(hd->Salt,SALT_SIZE);
				//hd->mtime.SetDos(hd->FileTime);
				//hd->ctime.Reset();
				//hd->atime.Reset();
				//hd->arctime.Reset();
				if (hd->Flags & LHD_EXTTIME)
				{
					ushort Flags;
					Raw.Get(Flags);
					//RarTime *tbl[4];
					//tbl[0]=&NewLhd.mtime;
					//tbl[1]=&NewLhd.ctime;
					//tbl[2]=&NewLhd.atime;
					//tbl[3]=&NewLhd.arctime;
					for (int I=0;I<4;I++)
					{
						//RarTime *CurTime=tbl[I];
						uint rmode=Flags>>(3-I)*4;
						if ((rmode & 8)==0)
							continue;
						if (I!=0)
						{
							uint DosTime;
							Raw.Get(DosTime);
							//CurTime->SetDos(DosTime);
						}
						
						/*
						struct {
							uint Year;
							uint Month;
							uint Day;
							uint Hour;
							uint Minute;
							uint Second;
							uint Reminder;
							uint wDay;
							uint yDay;
						} rlt;
						
						CurTime->GetLocal(&rlt);
						if (rmode & 4)
							rlt.Second++;
						rlt.Reminder=0;
						int count=rmode&3;
						for (int J=0;J<count;J++)
						{
							byte CurByte;
							Raw.Get(CurByte);
							rlt.Reminder|=(((uint)CurByte)<<((J+3-count)*8));
						}
						CurTime->SetLocal(&rlt);
						*/
						
						// skip time info
						for ( int n = rmode & 3; n--; )
						{
							byte CurByte;
							Raw.Get( CurByte );
						}
					}
				}
				NextBlockPos+=hd->FullPackSize;
				bool CRCProcessedOnly=(hd->Flags & LHD_COMMENT)!=0;
				HeaderCRC=~Raw.GetCRC(CRCProcessedOnly)&0xffff;
				if (hd->HeadCRC!=HeaderCRC)
					return "RAR file header CRC is incorrect";
			}
			break;
#ifndef SFX_MODULE
		case COMM_HEAD:
			*(BaseBlock *)&CommHead=ShortBlock;
			Raw.Get(CommHead.UnpSize);
			Raw.Get(CommHead.UnpVer);
			Raw.Get(CommHead.Method);
			Raw.Get(CommHead.CommCRC);
			break;
		case SIGN_HEAD:
			*(BaseBlock *)&SignHead=ShortBlock;
			Raw.Get(SignHead.CreationTime);
			Raw.Get(SignHead.ArcNameSize);
			Raw.Get(SignHead.UserNameSize);
			break;
		case AV_HEAD:
			*(BaseBlock *)&AVHead=ShortBlock;
			Raw.Get(AVHead.UnpVer);
			Raw.Get(AVHead.Method);
			Raw.Get(AVHead.AVVer);
			Raw.Get(AVHead.AVInfoCRC);
			break;
		case PROTECT_HEAD:
			*(BaseBlock *)&ProtectHead=ShortBlock;
			Raw.Get(ProtectHead.DataSize);
			Raw.Get(ProtectHead.Version);
			Raw.Get(ProtectHead.RecSectors);
			Raw.Get(ProtectHead.TotalBlocks);
			Raw.Get(ProtectHead.Mark,8);
			NextBlockPos+=ProtectHead.DataSize;
			break;
		case SUB_HEAD:
			*(BaseBlock *)&SubBlockHead=ShortBlock;
			Raw.Get(SubBlockHead.DataSize);
			NextBlockPos+=SubBlockHead.DataSize;
			Raw.Get(SubBlockHead.SubType);
			Raw.Get(SubBlockHead.Level);
			switch(SubBlockHead.SubType)
			{
				case UO_HEAD:
					*(SubBlockHeader *)&UOHead=SubBlockHead;
					Raw.Get(UOHead.OwnerNameSize);
					Raw.Get(UOHead.GroupNameSize);
					if (UOHead.OwnerNameSize>NM-1)
						UOHead.OwnerNameSize=NM-1;
					if (UOHead.GroupNameSize>NM-1)
						UOHead.GroupNameSize=NM-1;
					Raw.Get((byte *)UOHead.OwnerName,UOHead.OwnerNameSize);
					Raw.Get((byte *)UOHead.GroupName,UOHead.GroupNameSize);
					UOHead.OwnerName[UOHead.OwnerNameSize]=0;
					UOHead.GroupName[UOHead.GroupNameSize]=0;
					break;
				case MAC_HEAD:
					*(SubBlockHeader *)&MACHead=SubBlockHead;
					Raw.Get(MACHead.fileType);
					Raw.Get(MACHead.fileCreator);
					break;
				case EA_HEAD:
				case BEEA_HEAD:
				case NTACL_HEAD:
					*(SubBlockHeader *)&EAHead=SubBlockHead;
					Raw.Get(EAHead.UnpSize);
					Raw.Get(EAHead.UnpVer);
					Raw.Get(EAHead.Method);
					Raw.Get(EAHead.EACRC);
					break;
				case STREAM_HEAD:
					*(SubBlockHeader *)&StreamHead=SubBlockHead;
					Raw.Get(StreamHead.UnpSize);
					Raw.Get(StreamHead.UnpVer);
					Raw.Get(StreamHead.Method);
					Raw.Get(StreamHead.StreamCRC);
					Raw.Get(StreamHead.StreamNameSize);
					if (StreamHead.StreamNameSize>NM-1)
						StreamHead.StreamNameSize=NM-1;
					Raw.Get((byte *)StreamHead.StreamName,StreamHead.StreamNameSize);
					StreamHead.StreamName[StreamHead.StreamNameSize]=0;
					break;
			}
			break;
#endif
		default:
			if (ShortBlock.Flags & LONG_BLOCK)
			{
				uint DataSize;
				Raw.Get(DataSize);
				NextBlockPos+=DataSize;
			}
			break;
	}
	HeaderCRC=~Raw.GetCRC(false)&0xffff;
	CurHeaderType=ShortBlock.HeadType;

	if (NextBlockPos<=CurBlockPos)
		return "CRC error in RAR block header";
	
	return Raw.Size() ? NULL : end_of_rar;
}

const char* Archive::ReadOldHeader()
{
	RawRead Raw( rar_reader );
	if (CurBlockPos<=SFXSize)
	{
		Raw.Read(SIZEOF_OLDMHD);
		Raw.Get(OldMhd.Mark,4);
		Raw.Get(OldMhd.HeadSize);
		Raw.Get(OldMhd.Flags);
		NextBlockPos=CurBlockPos+OldMhd.HeadSize;
		CurHeaderType=MAIN_HEAD;
	}
	else
	{
		OldFileHeader OldLhd;
		Raw.Read(SIZEOF_OLDLHD);
		NewLhd.HeadType=FILE_HEAD;
		Raw.Get(NewLhd.PackSize);
		Raw.Get(NewLhd.UnpSize);
		Raw.Get(OldLhd.FileCRC);
		Raw.Get(NewLhd.HeadSize);
		Raw.Get(NewLhd.FileTime);
		Raw.Get(OldLhd.FileAttr);
		Raw.Get(OldLhd.Flags);
		Raw.Get(OldLhd.UnpVer);
		Raw.Get(OldLhd.NameSize);
		Raw.Get(OldLhd.Method);

		NewLhd.Flags=OldLhd.Flags|LONG_BLOCK;
		NewLhd.UnpVer=(OldLhd.UnpVer==2) ? 13 : 10;
		NewLhd.Method=OldLhd.Method+0x30;
		NewLhd.NameSize=OldLhd.NameSize;
		NewLhd.FileAttr=OldLhd.FileAttr;
		NewLhd.FileCRC=OldLhd.FileCRC;
		NewLhd.FullPackSize=NewLhd.PackSize;
		NewLhd.FullUnpSize=NewLhd.UnpSize;

		Raw.Read(OldLhd.NameSize);
		Raw.Get((byte *)NewLhd.FileName,OldLhd.NameSize);
		NewLhd.FileName[OldLhd.NameSize]=0;

		if (Raw.Size()!=0)
			NextBlockPos=CurBlockPos+NewLhd.HeadSize+NewLhd.PackSize;
		CurHeaderType=FILE_HEAD;
	}
	return (NextBlockPos > CurBlockPos && Raw.Size()) ? NULL : end_of_rar;
}

